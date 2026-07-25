// iggy3d_creative is the standalone creative-editor lab. It owns a small Vulkan
// app shell and drives the shared creative kernel directly: CreativeDocument,
// Facade tools/mutations, descriptor-driven placement, UI projection,
// wireframe/debug output, RoomBake preview, and save/open round-trip proof.
//
// This file remains the app integration surface. Keep authoring truth in the
// creative kernel and keep deterministic proof state in the extracted helpers:
// EditorBootstrap owns renderer/bootstrap data, EditorCapture owns the fixed
// capture schedule, and the Editor* helpers own picking, placement, gizmo/path
// editing, RoomBake preview, persistence proof, and app-local edit history.
//
// Rendered room geometry for bake-supported objects comes from RoomBake and the
// product scene projection. Standalone-only previews stay app-local: point
// markers are visual metadata for baked anchors, path/handle overlays have no
// runtime room geometry yet, and the green placement ghost is an editor affordance.
// The app may contain projection/hit-test glue, but object kind policy should
// continue to come from descriptors and shared kernel systems.

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <utility>

#include <SDL3/SDL.h>

#include "app/iggy3d/creative/render/CreativeSceneFrame.hpp"
#include "app/iggy3d/creative/world/MapTemplate.hpp"
#include "app/iggy3d/creative/world/WorldService.hpp"
#include "app/platform/SdlWindow.hpp"
#include "core/math/Vec3.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/FrameInput.hpp"
#include "render/vulkan/VulkanBackend.hpp"

#include "EditorAppFramePhases.hpp"
#include "EditorAssetLibrary.hpp"
#include "EditorCapture.hpp"
#include "EditorAssetReplacement.hpp"
#include "EditorAssets.hpp"
#include "EditorCatalog.hpp"
#include "EditorControls.hpp"
#include "EditorDesktopCommands.hpp"
#include "EditorDesktopPanels.hpp"
#include "EditorDesktopUi.hpp"
#include "EditorFrame.hpp"
#include "EditorFrustumCull.hpp"
#include "EditorGamepad.hpp"
#include "EditorGizmo.hpp"
#include "EditorGroup.hpp"
#include "EditorBootstrap.hpp"
#include "EditorInteraction.hpp"
#include "EditorMovingPlatformPreview.hpp"
#include "EditorState.hpp"
#include "EditorToolOptions.hpp"
#include "EditorToolWheelPreferences.hpp"
#include "EditorPlacement.hpp"
#include "EditorPersistence.hpp"
#include "EditorPlayerSpawnPreview.hpp"
#include "EditorPlaytestProcess.hpp"
#include "EditorPreviewFrame.hpp"
#include "EditorTransform.hpp"
#include "EditorTransformFrame.hpp"
#include "EditorVolume.hpp"
#include "EditorWorldLayoutLifecycle.hpp"
#include "EditorWorldLayoutBuildings.hpp"

namespace {

using namespace iggy3d;
using iggy3d_creative_app::activeCreativeEditorAppState;
using iggy3d_creative_app::buildCreativeEditorGizmoFrame;
using iggy3d_creative_app::buildCreativeEditorPickFrame;
using iggy3d_creative_app::buildStandaloneRoomBakePreviewScene;
using iggy3d_creative_app::captureFrameToPng;
using iggy3d_creative_app::createCreativeRenderer;
using iggy3d_creative_app::creativeDesktopShellEnabledForLaunch;
using iggy3d_creative_app::CreativeEditorBootstrapData;
using iggy3d_creative_app::CreativeEditorGamepad;
using iggy3d_creative_app::CreativeEditorState;
using iggy3d_creative_app::applyCreativeEditorCommandInput;
using iggy3d_creative_app::beginCreativeEditorAssetLibrary;
using iggy3d_creative_app::beginCreativeEditorAssetReplacement;
using iggy3d_creative_app::beginCreativeEditorFrameInput;
using iggy3d_creative_app::cancelCreativeEditorSelectionTransformPreview;
using iggy3d_creative_app::processCreativeEditorTransformFrame;
using iggy3d_creative_app::CreativeEditorFrameInputResult;
using iggy3d_creative_app::CreativeEditorGizmoFrame;
using iggy3d_creative_app::creativeEditorGroupFocusActive;
using iggy3d_creative_app::creativeEditorObjectInsideActiveGroup;
using iggy3d_creative_app::CreativeEditorPickFrame;
using iggy3d_creative_app::CreativeEditorSelectionFrame;
using iggy3d_creative_app::CreativeEditorOverlayFrame;
using iggy3d_creative_app::finishCreativeEditorVolumeHandleGesture;
using iggy3d_creative_app::ObjectVisualPickBounds;
using iggy3d_creative_app::firstBrushKind;
using iggy3d_creative_app::logCreativeEditorPathHandleCaptureFrame;
using iggy3d_creative_app::processCreativeEditorAssetLibraryFrame;
using iggy3d_creative_app::processCreativeEditorCatalogFrame;
using iggy3d_creative_app::processCreativeEditorAssetReplacementFrame;
using iggy3d_creative_app::processCreativeEditorControlsFrame;
using iggy3d_creative_app::processCreativeEditorToolOptionsFrame;
using iggy3d_creative_app::processCreativeEditorWorldInteractionFrame;
using iggy3d_creative_app::submitCreativeEditorFrame;
using iggy3d_creative_app::logCreativeEditorWorldPickProofFrame;
using iggy3d_creative_app::resolveCreativeEditorSelectionFrame;
using iggy3d_creative_app::runCreativeEditorCaptureScenarioFrame;
using iggy3d_creative_app::syncCreativeEditorGroupFocus;
using iggy3d_creative_app::StandaloneRoomBakePreviewScene;
using iggy3d_creative_app::initializeCreativeEditorBootstrapData;
using iggy3d_creative_app::loadStandaloneScene;
using iggy3d_creative_app::markCreativeEditorDocumentSaved;
using iggy3d_creative_app::reloadCreativeEditorAssets;

std::filesystem::path creativeStandaloneSaveRoot() {
  if (const char* home = std::getenv("HOME"); home != nullptr) {
    return std::filesystem::path{home} / ".iggy3d" / "creative_standalone";
  }
  return std::filesystem::path{".iggy3d"} / "creative_standalone";
}

bool generateCreativeMapSave(std::string_view templateId) {
  if (!creative::isCreativeMapTemplateId(templateId)) {
    std::fprintf(stderr, "iggy3d_creative: unknown map template '%.*s'\n",
                 static_cast<int>(templateId.size()), templateId.data());
    return false;
  }

  creative::CreativeMapTemplateResult map =
      creative::buildCreativeMapTemplate(templateId);
  if (!map.accepted) {
    std::fprintf(stderr,
                 "iggy3d_creative: map generation failed reason='%.*s'\n",
                 static_cast<int>(map.reasonCode.size()),
                 map.reasonCode.data());
    return false;
  }

  if (!map.buildingTemplates.empty()) {
    iggy3d_creative_app::CreativeEditorWorldLayoutBuildingTemplateLibrary
        library;
    const auto loaded =
        iggy3d_creative_app::loadCreativeEditorWorldLayoutBuildingTemplateLibrary(
            library, creativeStandaloneSaveRoot());
    if (!loaded.accepted) {
      std::fprintf(stderr,
                   "iggy3d_creative: building template library failed "
                   "reason='%s'\n",
                   loaded.reasonCode.c_str());
      return false;
    }
    for (const creative::CreativeWorldLayoutBuildingTemplate& source :
         map.buildingTemplates) {
      const auto installed =
          iggy3d_creative_app::
              installCreativeEditorBuiltInWorldLayoutBuildingTemplate(
              library, source);
      if (!installed.accepted) {
        std::fprintf(stderr,
                     "iggy3d_creative: building template install failed "
                     "reason='%s'\n",
                     installed.reasonCode.c_str());
        return false;
      }
    }
  }

  CreativeWorldSaveRequest request;
  request.saveRoot = creativeStandaloneSaveRoot();
  request.saveId = std::string{templateId};
  request.document = &map.document;
  request.worldTitle = std::string(map.document.name());
  request.saveTitle = request.worldTitle;
  request.saveType = "creative";
  request.worldLayout = map.worldLayoutPresent ? &map.worldLayout : nullptr;
  const CreativeWorldSaveResult saved = saveCreativeWorld(request);
  if (!saved.accepted) {
    std::fprintf(stderr,
                 "iggy3d_creative: map save failed reason='%s'\n",
                 saved.reasonCode.c_str());
    return false;
  }

  std::fprintf(stdout,
               "iggy3d_creative: generated map='%.*s' objects=%llu "
               "terrain_controls=%llu path='%s'\n",
               static_cast<int>(templateId.size()), templateId.data(),
               static_cast<unsigned long long>(map.objectCount),
               static_cast<unsigned long long>(map.terrainControlCount),
               saved.path.generic_string().c_str());
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  // Optional flags:
  //   --frames N        auto-exit after N presented frames (scriptable run)
  //   --capture <path>  render a few frames, write <path>.png, then exit
  //   --map <template>  generate if absent, then open a built-in map
  //   --load <save-id>  open an existing Creative save slot
  //   --generate-map <template>  write a map save headlessly, then exit
  //   --desktop-ui      opt into the docked IDE shell
  std::uint64_t maxFrames = 0;  // 0 = run until window close.
  std::string capturePath;
  std::string mapTemplateId;
  std::string loadSaveId;
  std::string generateMapTemplateId;
  bool desktopUiRequested = false;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--frames" && i + 1 < argc) {
      maxFrames = std::strtoull(argv[++i], nullptr, 10);
    } else if (arg == "--capture" && i + 1 < argc) {
      capturePath = argv[++i];
    } else if (arg == "--map" && i + 1 < argc) {
      mapTemplateId = argv[++i];
    } else if (arg == "--load" && i + 1 < argc) {
      loadSaveId = argv[++i];
    } else if (arg == "--generate-map" && i + 1 < argc) {
      generateMapTemplateId = argv[++i];
    } else if (arg == "--desktop-ui") {
      desktopUiRequested = true;
    }
  }
  if (!generateMapTemplateId.empty()) {
    return generateCreativeMapSave(generateMapTemplateId) ? 0 : 1;
  }
  if (!mapTemplateId.empty()) {
    const CreativeWorldOpenResult existing = openCreativeWorld(
        {creativeStandaloneSaveRoot(), mapTemplateId});
    if (!existing.accepted && !generateCreativeMapSave(mapTemplateId)) {
      return 1;
    }
    loadSaveId = mapTemplateId;
  }
  // In capture mode, render a handful of frames to let the swapchain settle,
  // then grab the last one.
  if (!capturePath.empty() && maxFrames == 0U) {
    maxFrames = 8U;
  }
  const bool captureMode = !capturePath.empty();
  const bool desktopShellEnabled = creativeDesktopShellEnabledForLaunch(
      desktopUiRequested, captureMode);

  // Window (Vulkan). The SdlWindow ctor initializes the SDL video subsystem.
  SdlWindowCreateInfo createInfo;
  createInfo.title = "iggy3d creative";
  createInfo.width = 1280;
  createInfo.height = 720;
  createInfo.resizable = true;
  createInfo.highDpi = true;
  createInfo.vulkan = true;
  SdlWindow window(createInfo);
  if (!window.isOpen()) {
    SDL_Log("iggy3d_creative: failed to open window");
    return 1;
  }

  std::unique_ptr<VulkanBackend> backend =
      createCreativeRenderer(window, desktopShellEnabled);
  if (backend == nullptr ||
      backend->lifecycleState() != RendererLifecycleState::Ready) {
    SDL_Log("iggy3d_creative: renderer not ready (lifecycle=%d)",
            backend == nullptr ? -1
                               : static_cast<int>(backend->lifecycleState()));
    return 1;
  }

  // The external UI event feed and free-pointer policy exist only for an
  // explicit --desktop-ui launch. Plain i3dc keeps the original full-viewport
  // relative-mouse editor behavior.
  if (desktopShellEnabled) {
    SdlWindowEventHook desktopUiEventHook;
    desktopUiEventHook.onEvent = [](void* context, const SDL_Event& event) {
      static_cast<VulkanBackend*>(context)->forwardExternalUiEvent(event);
    };
    desktopUiEventHook.context = backend.get();
    window.setEventHook(desktopUiEventHook);
    // Desktop mode rests on a free cursor: the shell owns pointer capture, so
    // the ~15 modal-close re-grab sites and the startup grab below are
    // suppressed until a viewport click enters fly-look (plan DD-9).
    window.setDesktopFreePointerMode(true);
  }

  // Relative mouse mode for a free-look fly camera (interactive only — don't
  // grab the mouse during a scripted --capture run). In desktop mode this is
  // suppressed by the free-pointer gate above; click-to-capture enters fly-look.
  if (!captureMode) {
    window.setRelativeMouseMode(true);
  }

  CreativeEditorBootstrapData bootstrapData;
  initializeCreativeEditorBootstrapData(
      bootstrapData, captureMode,
      /*seedStarterScene=*/!desktopShellEnabled || captureMode);
  CreativeEditorState& editor = bootstrapData.editor;
  editor.desktopUi.shellEnabled = desktopShellEnabled;
  creative::CreativeAppState& appState = bootstrapData.appState;
  markCreativeEditorDocumentSaved(
      editor.persistence, appState.facade.document());
  creative::CreativeObjectId floorObjectId = bootstrapData.floorObjectId;
  const std::filesystem::path& saveRoot = bootstrapData.saveRoot;
  std::string saveId = bootstrapData.saveId;
  if (!loadSaveId.empty()) {
    saveId = loadSaveId;
    creative::CreativeWorldLayout loadedLayout;
    if (!loadStandaloneScene(appState, saveRoot, saveId, &loadedLayout)) {
      SDL_Log("iggy3d_creative: startup load failed saveId='%s'",
              saveId.c_str());
      return 1;
    }
    installCreativeEditorWorldLayout(editor.worldLayout,
                                     std::move(loadedLayout));
    markCreativeEditorDocumentSaved(
        editor.persistence, appState.facade.document());
    floorObjectId = iggy3d_creative_app::firstCreativeEditorFloorObjectId(
        appState.facade.document());
    if (mapTemplateId == creative::kDitchHouseMapTemplateId) {
      editor.flyPos = {4.0F, 16.0F, 34.0F};
      editor.yawDegrees = 18.0F;
      editor.pitchDegrees = -22.0F;
    } else if (mapTemplateId == creative::kBuilderEstateMapTemplateId) {
      editor.flyPos = {0.0F, 24.0F, 36.0F};
      editor.yawDegrees = 0.0F;
      editor.pitchDegrees = -28.0F;
    }
  }
  const std::filesystem::path controlsPath =
      saveRoot / "creative_controls_v1.cfg";
  const std::filesystem::path toolWheelPath =
      saveRoot / "creative_tool_wheel_v1.cfg";
  static_cast<void>(iggy3d_creative_app::loadCreativeEditorControlProfile(
      editor.controlProfile, controlsPath,
      &editor.playtestWindowPreferences));
  static_cast<void>(iggy3d_creative_app::loadCreativeEditorToolWheel(
      editor.catalog.toolWheel, editor.catalog.model, toolWheelPath));
  const creative::CreativeSpatialProjectionRequest& wireProjReq =
      bootstrapData.wireProjectionRequest;
  const float kGizmoAxisLength = bootstrapData.gizmoAxisLengthMeters;
  const float kGizmoThickness = bootstrapData.gizmoThicknessMeters;
  CreativeEditorGamepad gamepad;
  iggy3d_creative_app::CreativeEditorSceneCache sceneCache;
  iggy3d_creative_app::CreativeEditorGeneratedTerrainPreviewCache
      terrainOperationPreviewCache;
  iggy3d_creative_app::CreativeEditorVolumeScenePreviewCache
      volumeScenePreviewCache;
  iggy3d_creative_app::CreativePlayerSpawnPreviewCache
      playerSpawnPreviewCache;
  // The one playtest child this editor may own (kill-reap-snapshot-spawn on
  // Play, reaped on editor exit, polled non-blocking every frame).
  iggy3d_creative_app::PlaytestProcessOwner playtestOwner;

  while (window.isOpen()) {
    iggy3d_creative_app::CreativeEditorAppInputPhaseReceipt inputPhase =
        iggy3d_creative_app::runCreativeEditorAppInputPhase(
            {window,
             *backend,
             gamepad,
             editor,
             appState,
             playtestOwner,
             !capturePath.empty()});
    if (inputPhase.disposition ==
        iggy3d_creative_app::CreativeEditorAppFrameDisposition::Stop) {
      break;
    }
    if (inputPhase.disposition ==
        iggy3d_creative_app::CreativeEditorAppFrameDisposition::Skip) {
      continue;
    }
    CreativeEditorFrameInputResult& frameInput = inputPhase.frameInput;
    const SdlDrawableExtent extent = frameInput.extent;
    const iggy3d_creative_app::CreativeEditorAppToolPhaseReceipt toolPhase =
        iggy3d_creative_app::runCreativeEditorAppToolPhase(
            {window,
             *backend,
             appState,
             editor,
             frameInput,
             saveRoot,
             saveId,
             controlsPath,
             toolWheelPath,
             bootstrapData.staticMeshAssetCatalog,
             bootstrapData.assetRoot,
             playtestOwner,
             sceneCache,
             terrainOperationPreviewCache,
             floorObjectId});
    creative::CreativeAppState& activeAppState = *toolPhase.activeAppState;
    const bool modalBlocksWorldActions =
        toolPhase.modalBlocksWorldActions;
    const iggy3d_creative_app::CreativeEditorAppPreviewPhaseRequest
        previewPhaseRequest{
            activeAppState,
            editor,
            sceneCache,
            terrainOperationPreviewCache,
            volumeScenePreviewCache,
            bootstrapData.staticMeshAssetCatalog,
            modalBlocksWorldActions,
            !capturePath.empty()};
    iggy3d_creative_app::CreativeEditorAppPreviewPhaseReceipt previewPhase =
        iggy3d_creative_app::prepareCreativeEditorAppPreviewPhase(
            previewPhaseRequest);
    SceneProjectionResult& scene = previewPhase.selectedPreview->scene;
    DebugProjectionResult debug{};

    // FRAME (non-const so we can attach UI + wireframe + label below). This
    // gives frame.camera.clipFromWorld (world -> NDC) for click + label maths.
    FrameInput frame = makeCreativeVulkanFrame(
        scene, debug, editor.frameIndex++, extent.width, extent.height,
        editor.yawDegrees, editor.pitchDegrees,
        /*cameraAnchorOverrideAvailable=*/true, editor.flyPos,
        editor.desktopUi.contentViewport);

    // Scan every visible object's visual bounds once. Live interaction resolves
    // the center ray from this frame; scripted capture retains its fixed proof ray.
    static_cast<void>(syncCreativeEditorGroupFocus(
        editor.groupFocus, activeAppState.facade.document()));
    CreativeEditorPickFrame pickFrame = buildCreativeEditorPickFrame(
        activeAppState.facade.document(),
        frame.camera,
        extent.width,
        extent.height,
        editor.interaction.movingPlatformPathEdit.objectId,
        editor.interaction.structuralSpanEdit.objectId,
        editor.assetEdit.active ? creative::kInvalidObjectId : floorObjectId,
        editor.captureScript,
        !capturePath.empty());
    if (creativeEditorGroupFocusActive(editor.groupFocus)) {
      std::erase_if(
          pickFrame.objectPickCandidates,
          [&](const ObjectVisualPickBounds& candidate) {
            return !creativeEditorObjectInsideActiveGroup(
                activeAppState.facade.document(), editor.groupFocus,
                candidate.id);
          });
    }
    logCreativeEditorWorldPickProofFrame(activeAppState.facade,
                                         frame.camera,
                                         extent.width,
                                         extent.height,
                                         pickFrame,
                                         editor.assetEdit.active
                                             ? creative::kInvalidObjectId
                                             : floorObjectId,
                                         editor,
                                         !capturePath.empty());

    const RenderContentViewport contentViewport =
        effectiveContentViewport(frame);
    const CreativeEditorSelectionFrame interactionSelection =
        resolveCreativeEditorSelectionFrame(activeAppState.facade);
    const CreativeEditorGizmoFrame interactionGizmo =
        buildCreativeEditorGizmoFrame(
            interactionSelection,
            editor.interaction.movingPlatformPathEdit, frame.camera,
            extent.width, extent.height, kGizmoAxisLength, contentViewport,
            &editor.transform);

    if (!modalBlocksWorldActions && frameInput.windowFocused) {
      processCreativeEditorWorldInteractionFrame(
          {activeAppState,
           editor,
           frameInput.worldActions,
           frameInput.modifiers,
           frame.camera,
           pickFrame,
           effectiveContentViewport(frame),
           frameInput.monotonicTimeNanoseconds,
           !capturePath.empty(),
           &bootstrapData.staticMeshAssetCatalog,
           &sceneCache.placementClearance,
           &interactionGizmo});
    } else if (editor.volume.handleGesture.active) {
      static_cast<void>(
          finishCreativeEditorVolumeHandleGesture(editor.volume, false));
    }

    runCreativeEditorCaptureScenarioFrame(
        appState, editor, saveRoot, saveId, !capturePath.empty());

    iggy3d_creative_app::refreshCreativeEditorAppPreviewAfterInteraction(
        previewPhaseRequest, previewPhase, frame);

    if (iggy3d_creative_app::runCreativeEditorAppRenderPhase(
            {*backend,
             activeAppState,
             editor,
             sceneCache,
             terrainOperationPreviewCache,
             volumeScenePreviewCache,
             playerSpawnPreviewCache,
             previewPhase,
             frame,
             wireProjReq,
             frameInput,
             bootstrapData.staticMeshAssetCatalog,
             extent.width,
             extent.height,
             kGizmoAxisLength,
             kGizmoThickness,
             !capturePath.empty(),
             maxFrames})) {
      break;
    }
  }

  creative::CreativeAppState& shutdownAppState =
      activeCreativeEditorAppState(editor, appState);
  finalizeCreativeEditorContinuousGestures(
      shutdownAppState, editor, "creative_continuous_gesture_shutdown");
  static_cast<void>(iggy3d_creative_app::cancelCreativeEditorAuthoredAssetEdit(
      editor, "creative_authored_asset_edit_shutdown"));
  static_cast<void>(iggy3d_creative_app::cancelCreativeEditorAssetReplacement(
      editor.assetReplacement, "creative_asset_replace_shutdown"));
  // Editor exit reaps the child: no orphaned playtest windows.
  playtestOwner.shutdown();

  bool captureOk = true;
  if (!capturePath.empty()) {
    captureOk = captureFrameToPng(*backend, capturePath);
  }
  backend->waitIdle();
  backend->shutdown();
  return captureOk ? 0 : 1;
}
