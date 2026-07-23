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
#include "app/iggy3d/creative/play/PlaySession.hpp"
#include "EditorPreviewFrame.hpp"
#include "EditorTransform.hpp"
#include "EditorVolume.hpp"
#include "EditorWorldLayout.hpp"

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

creative::CreativeObjectId firstFloorObjectId(
    const creative::CreativeDocument& document) noexcept {
  for (const creative::CreativeObject& object : document.objects()) {
    if (object.kind == creative::CreativeObjectKind::Floor) {
      return object.id;
    }
  }
  return creative::kInvalidObjectId;
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
    floorObjectId = firstFloorObjectId(appState.facade.document());
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
  iggy3d_creative_app::CreativePlaySession playMode;
  // The one playtest child this editor may own (kill-reap-snapshot-spawn on
  // Play, reaped on editor exit, polled non-blocking every frame).
  iggy3d_creative_app::PlaytestProcessOwner playtestOwner;

  while (window.isOpen()) {
    const iggy3d_creative_app::PlaytestProcessOwner::PollResult playtestPoll =
        playtestOwner.poll();
    editor.desktopUi.playtestRunning = playtestPoll.running;
    if (playtestPoll.exitObserved) {
      editor.desktopUi.statusMessage =
          playtestOwner.monitor().lastExitMessage;
    } else if (playtestPoll.stalled) {
      // Live age display while stalled; no auto-kill -- Play replaces.
      editor.desktopUi.statusMessage =
          iggy3d_creative_app::playtestStalledStatusMessage(
              playtestPoll.stallAgeMs);
    } else if (playtestPoll.stallRecovered) {
      editor.desktopUi.statusMessage = "playtest recovered";
    }
    {
      // Non-applied acks surface in the status bar (applied ones are
      // visible in the Play Monitor's ack row).
      const iggy3d_creative_app::PlaytestMonitorState& playtestMonitor =
          playtestOwner.monitor();
      static std::uint64_t lastSurfacedAckSeq = 0U;
      if (playtestMonitor.lastAckSeq != 0U &&
          playtestMonitor.lastAckSeq != lastSurfacedAckSeq &&
          playtestMonitor.lastAckStatus != "applied") {
        lastSurfacedAckSeq = playtestMonitor.lastAckSeq;
        editor.desktopUi.statusMessage =
            "playtest " + playtestMonitor.lastAckVerb + ": " +
            playtestMonitor.lastAckStatus +
            (playtestMonitor.lastAckReason.empty()
                 ? std::string{}
                 : " (" + playtestMonitor.lastAckReason + ")");
      }
    }
    // Non-const: captured Esc release consumes the ToggleControls action from
    // the route this frame so Controls does not also open (plan DD-9 / FC-4).
    CreativeEditorFrameInputResult frameInput = beginCreativeEditorFrameInput(
        window, *backend, gamepad, editor, !capturePath.empty(),
        /*applyEditorNavigation=*/true);
    if (!frameInput.keepRunning) {
      break;
    }
    if (frameInput.skipFrame) {
      if (!frameInput.windowFocused) {
        creative::CreativeAppState& activeAppState =
            activeCreativeEditorAppState(editor, appState);
        finalizeCreativeEditorContinuousGestures(
            activeAppState, editor, "creative_continuous_gesture_focus_lost");
        if (cancelCreativeEditorSelectionTransformPreview(
                editor.transform, "selection_transform_focus_lost")) {
          static_cast<void>(window.setRelativeMouseMode(true));
        }
        static_cast<void>(iggy3d_creative_app::
                              cancelCreativeEditorAssetReplacement(
                                  editor.assetReplacement,
                                  "creative_asset_replace_focus_lost"));
      }
      continue;
    }
    const SdlDrawableExtent extent = frameInput.extent;
    if (!frameInput.windowFocused &&
        cancelCreativeEditorSelectionTransformPreview(
            editor.transform, "selection_transform_focus_lost")) {
      static_cast<void>(window.setRelativeMouseMode(true));
    }
    if (!frameInput.windowFocused) {
      static_cast<void>(iggy3d_creative_app::
                            cancelCreativeEditorAssetReplacement(
                                editor.assetReplacement,
                                "creative_asset_replace_focus_lost"));
    }
    // The ImGui frame's NewFrame already ran inside beginCreativeEditorFrameInput
    // (NewFrame-before-context); here we only build the dockspace + content rect.
    iggy3d_creative_app::layoutCreativeEditorDesktopDockspace(editor.desktopUi);
    // Free-pointer capture policy (DD-9). Runs after the dockspace's NewFrame
    // so externalUiWantsMouse() reflects whether this frame's click landed on
    // a panel; a viewport click enters fly-look, leaving the viewport releases.
    {
      // Captured Esc = release only: while flying, Esc frees the pointer
      // WITHOUT opening the Controls panel. Consume the ToggleControls action
      // this frame so no downstream panel acts on it.
      const bool capturedEscRelease =
          iggy3d_creative_app::creativeDesktopPointerCaptured(
              editor.desktopUi.viewportPointerCaptureMode) &&
          creative::creativeInputRouteContains(
              frameInput.routedInput,
              creative::CreativeInputActionId::ToggleControls);
      if (capturedEscRelease) {
        editor.desktopUi.viewportPointerCaptureMode =
            iggy3d_creative_app::CreativeDesktopPointerCaptureMode::None;
        static_cast<void>(window.setViewportPointerCapture(false));
        creative::creativeInputRouteRemove(
            frameInput.routedInput,
            creative::CreativeInputActionId::ToggleControls);
      } else {
        const bool primaryOverViewport =
            window.eventState().primaryPointerPressed &&
            !backend->externalUiWantsMouse();
        const bool viewportContext =
            frameInput.routedInput.context ==
                creative::CreativeInputContext::EditorViewport ||
            frameInput.routedInput.context ==
                creative::CreativeInputContext::RuntimePlay;
        const creative::CreativeInputModifierMask viewportGestureModifiers =
            frameInput.routedInput.context ==
                    creative::CreativeInputContext::EditorViewport
                ? frameInput.modifiers
                : creative::kCreativeInputModifierNone;
        const iggy3d_creative_app::CreativeDesktopPointerDecision
            pointerDecision =
                iggy3d_creative_app::decideCreativeDesktopPointerCapture(
                    editor.desktopUi.shellEnabled,
                    editor.desktopUi.viewportPointerCaptureMode,
                    primaryOverViewport,
                    creative::creativeInputKeyDown(
                        frameInput.inputFrame,
                        creative::CreativeInputKey::MousePrimary),
                    viewportGestureModifiers, viewportContext,
                    frameInput.windowFocused);
        const bool gestureOwnsPrimary =
            iggy3d_creative_app::creativeDesktopPointerModeOwnsPrimaryAction(
                editor.desktopUi.viewportPointerCaptureMode) ||
            iggy3d_creative_app::creativeDesktopPointerModeOwnsPrimaryAction(
                pointerDecision.mode);
        if (pointerDecision.consumePrimaryPress || gestureOwnsPrimary) {
          // Click-to-capture owns this press. Do not also remove/place/select in
          // the editor or attack in Play on the frame fly-look is acquired.
          creative::setCreativeInputKey(
              frameInput.inputFrame,
              creative::CreativeInputKey::MousePrimary, false);
          creative::creativeInputRouteRemove(
              frameInput.routedInput,
              creative::CreativeInputActionId::PrimaryAction);
          creative::creativeInputRouteRemove(
              frameInput.routedInput,
              creative::CreativeInputActionId::RuntimeAttack);
          constexpr std::size_t primaryActionIndex =
              static_cast<std::size_t>(
                  creative::CreativeWorldActionId::Primary);
          frameInput.worldActions.down[primaryActionIndex] = false;
          frameInput.worldActions.pressed[primaryActionIndex] = false;
          frameInput.worldActions.released[primaryActionIndex] = false;
        }
        if (pointerDecision.changed) {
          const bool wasCaptured =
              iggy3d_creative_app::creativeDesktopPointerCaptured(
                  editor.desktopUi.viewportPointerCaptureMode);
          const SdlMouseCaptureResult captureResult =
              window.setViewportPointerCapture(pointerDecision.captured);
          const bool acquired = !wasCaptured && captureResult.active;
          editor.desktopUi.viewportPointerCaptureMode =
              captureResult.active
                  ? pointerDecision.mode
                  : iggy3d_creative_app::
                        CreativeDesktopPointerCaptureMode::None;
          editor.desktopUi.discardNextViewportMouseDelta =
              acquired && pointerDecision.discardNextMouseDelta;
        }
      }
    }
    iggy3d_creative_app::CreativeEditorAssetLibraryFrameResult
        assetLibraryFrame;
    assetLibraryFrame.remainingInput = frameInput.routedInput;
    assetLibraryFrame = processCreativeEditorAssetLibraryFrame(
        {window, appState, editor, frameInput.routedInput});
    if (assetLibraryFrame.activeDocumentChanged) {
      invalidateCreativeEditorSceneCache(sceneCache);
    }
    creative::CreativeAppState& activeAppState =
        iggy3d_creative_app::creativePlaySessionActive(playMode)
            ? appState
            : activeCreativeEditorAppState(editor, appState);
    if (synchronizeCreativeEditorTerrainGeneration(
            editor.terrainGeneration, activeAppState.facade.document())) {
      invalidateCreativeEditorGeneratedTerrainPreview(
          terrainOperationPreviewCache);
    }

    // Desktop shell chrome: the menu emits semantic command IDs, the sole
    // dispatcher applies them to the existing kernels, and the status bar
    // reads the result. Only runs when the shell is active (never under
    // --capture, so image output is unchanged). DL-3: widgets emit, the
    // dispatcher mutates.
    if (editor.desktopUi.frameActive) {
      iggy3d_creative_app::CreativeDesktopCommandFrame desktopCommands;
      iggy3d_creative_app::buildCreativeEditorDesktopMenuBar(
          editor.desktopUi, activeAppState,
          &activeAppState == &appState ? &editor.worldLayout : nullptr,
          iggy3d_creative_app::creativePlaySessionActive(playMode),
          desktopCommands);
      iggy3d_creative_app::buildCreativeEditorDesktopPanels(
          editor.desktopUi, editor, activeAppState,
          &bootstrapData.staticMeshAssetCatalog, &playMode,
          &playtestOwner.monitor(), assetLibraryFrame.remainingInput,
          desktopCommands);
      if (desktopCommands.count > 0U) {
        const iggy3d_creative_app::CreativeDesktopCommandResult desktopResult =
            iggy3d_creative_app::dispatchCreativeDesktopCommands(
                desktopCommands,
                {appState,
                 editor,
                 saveRoot,
                 &saveId,
                 &playMode,
                 &bootstrapData.staticMeshAssetCatalog,
                 &playtestOwner});
        if (!desktopResult.message.empty()) {
          editor.desktopUi.statusMessage = desktopResult.message;
        }
        if (desktopResult.documentReplaced || desktopResult.sceneChanged) {
          invalidateCreativeEditorSceneCache(sceneCache);
          floorObjectId = firstFloorObjectId(appState.facade.document());
        }
      }
      iggy3d_creative_app::buildCreativeEditorDesktopStatusBar(
          editor.desktopUi, editor, activeAppState);
    }

    const creative::CreativeInputRouteResult& routedInput =
        assetLibraryFrame.remainingInput;
    const iggy3d_creative_app::CreativeEditorControlsFrameResult controlsFrame =
        processCreativeEditorControlsFrame(
            {window,
             editor,
             routedInput,
             frameInput.inputFrame,
             controlsPath,
             toolWheelPath,
             frameInput.monotonicTimeNanoseconds,
             extent.width,
             extent.height});
    const iggy3d_creative_app::CreativeEditorTransformFrameResult
        transformFrame = processCreativeEditorTransformFrame(
            {window,
             activeAppState,
             editor,
             routedInput,
             frameInput.toolWheelDirectionX,
             frameInput.toolWheelDirectionY,
             frameInput.transformNudgeWheelSteps,
             frameInput.transformFineNudge,
             extent.width,
             extent.height});
    const iggy3d_creative_app::CreativeEditorCatalogFrameResult catalogFrame =
        processCreativeEditorCatalogFrame(
            {window,
             activeAppState,
             editor,
             routedInput,
             frameInput.worldActions,
             toolWheelPath,
             frameInput.toolWheelDirectionX,
             frameInput.toolWheelDirectionY,
             extent.width,
             extent.height});
    if (catalogFrame.authoredAssetLibraryRequested) {
      const bool opened = beginCreativeEditorAssetLibrary(
          editor, appState.facade.document(), catalogFrame.authoredAssetId);
      if (opened) {
        static_cast<void>(creative::setCreativeCatalogOpen(
            editor.catalog.model, false));
        static_cast<void>(window.setTextInputActive(true));
        static_cast<void>(window.setRelativeMouseMode(false));
      } else {
        editor.catalog.statusLabel = "AUTHORED ASSET LIBRARY UNAVAILABLE";
      }
    }
    if (catalogFrame.assetReplacementRequested) {
      const iggy3d_creative_app::CreativeAssetReplacementBeginReceipt begun =
          beginCreativeEditorAssetReplacement(
              activeAppState, bootstrapData.staticMeshAssetCatalog,
              catalogFrame.replacementObjectKind,
              catalogFrame.replacementAssetId, editor.assetReplacement);
      if (begun.accepted) {
        static_cast<void>(creative::setCreativeCatalogOpen(
            editor.catalog.model, false));
        static_cast<void>(window.setTextInputActive(false));
        static_cast<void>(window.setRelativeMouseMode(true));
      } else {
        editor.catalog.statusLabel =
            "REPLACE FAILED: " + std::string(begun.reasonCode);
      }
    }
    const iggy3d_creative_app::CreativeEditorAssetReplacementFrameResult
        assetReplacementFrame = processCreativeEditorAssetReplacementFrame(
            {activeAppState, editor.assetReplacement, routedInput});
    if (assetReplacementFrame.finished &&
        !assetReplacementFrame.commitReceipt.accepted) {
      invalidateCreativeEditorSceneCache(sceneCache);
    }
    const iggy3d_creative_app::CreativeEditorToolOptionsFrameResult
        toolOptionsFrame = processCreativeEditorToolOptionsFrame(
            {window,
             activeAppState,
             editor,
             routedInput,
             catalogFrame.openToolOptionsRequested,
             catalogFrame.toolOptionsEntry,
             extent.width,
             extent.height,
             &bootstrapData.staticMeshAssetCatalog,
             &sceneCache.placementClearance});
    const bool layoutPreviewActive =
        creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
    const bool modalBlocksWorldActions =
        assetLibraryFrame.blockWorldActions ||
        controlsFrame.blockWorldActions || transformFrame.blockWorldActions ||
        catalogFrame.blockWorldActions ||
        assetReplacementFrame.blockWorldActions ||
        toolOptionsFrame.blockWorldActions || layoutPreviewActive ||
        editor.terrainGeneration.previewActive;
    if (modalBlocksWorldActions || !frameInput.windowFocused) {
      finalizeCreativeEditorContinuousGestures(
          activeAppState, editor,
          frameInput.windowFocused ? "creative_continuous_gesture_modal"
                                   : "creative_continuous_gesture_focus_lost");
    }
    if (catalogFrame.assetReloadRequested) {
      static_cast<void>(reloadCreativeEditorAssets(
          {*backend,
           activeAppState,
           editor,
           sceneCache,
           bootstrapData.staticMeshAssetCatalog,
           bootstrapData.assetRoot}));
    }
    if (!catalogFrame.deferredCommandInput.actionEvents().empty()) {
      applyCreativeEditorCommandInput(catalogFrame.deferredCommandInput,
                                      activeAppState, editor, saveRoot, saveId);
    }
    if (!modalBlocksWorldActions && frameInput.windowFocused) {
      applyCreativeEditorCommandInput(
          routedInput, activeAppState, editor, saveRoot, saveId);
    }

    // SCENE (local, must outlive submitFrame): bake supported room geometry
    // through the same CreativeDocument -> RoomAsset adapter that gameplay will
    // eventually consume, then project that RoomAsset through the runtime scene
    // path. Standalone-only editor proxies remain only for objects that RoomBake
    // did not emit as static geometry, such as Point anchors and Path routes.
    const creative::CreativeDocument& layoutRenderDocument =
        creativeEditorWorldLayoutRenderDocument(
            editor.worldLayout, activeAppState.facade.document());
    const creative::CreativeDocument& renderDocument =
        iggy3d_creative_app::creativeEditorAssetReplacementRenderDocument(
            editor.assetReplacement, layoutRenderDocument);
    static_cast<void>(refreshCreativeEditorSceneCache(
        sceneCache, renderDocument,
        &bootstrapData.staticMeshAssetCatalog));
    const auto refreshVolumeScenePreview = [&]() {
      const creative::CreativeHotbarEntry& held =
          creative::selectedCreativeHotbarEntry(editor.interaction.hotbar);
      const bool fillPreview =
          held.kind == creative::CreativeHeldItemKind::VolumeFill;
      const bool clonePreview =
          held.kind == creative::CreativeHeldItemKind::VolumeClone;
      const bool eligible =
          &renderDocument == &activeAppState.facade.document() &&
          (fillPreview || clonePreview) &&
          editor.volume.active && !modalBlocksWorldActions &&
          capturePath.empty();
      if (!eligible) {
        invalidateCreativeEditorVolumeScenePreview(volumeScenePreviewCache);
        return;
      }
      editor.volume.operation =
          fillPreview ? creative::CreativeVolumeOperationKind::Fill
                      : creative::CreativeVolumeOperationKind::Clone;
      const creative::CreativeVolumeSelection selection =
          creativeEditorVolumePreviewSelection(editor.volume);
      if (!creative::creativeVolumeSelectionValid(selection)) {
        invalidateCreativeEditorVolumeScenePreview(volumeScenePreviewCache);
        return;
      }
      static_cast<void>(refreshCreativeEditorVolumeOperationPreview(
          editor.volume, renderDocument, selection, editor.placeBrush,
          editor.toolSettings));
      if (!editor.volume.preview.stagedDocumentValid) {
        invalidateCreativeEditorVolumeScenePreview(volumeScenePreviewCache);
        return;
      }
      static_cast<void>(refreshCreativeEditorVolumeScenePreview(
          volumeScenePreviewCache, editor.volume.preview.stagedDocument,
          editor.volume.preview.refreshCount,
          &bootstrapData.staticMeshAssetCatalog));
    };
    refreshVolumeScenePreview();
    const auto refreshTerrainOperationScenePreview = [&]() {
      const creative::CreativeTerrainOperationMutationPlan* operationPreview =
          nullptr;
      const bool targetsRenderedDocument =
          &renderDocument == &activeAppState.facade.document();
      if (targetsRenderedDocument &&
          creativeEditorTerrainGenerationPreviewMatches(
              editor.terrainGeneration, renderDocument)) {
        operationPreview = &editor.terrainGeneration.operationPreview;
      }

      const creative::CreativeHotbarEntry& held =
          creative::selectedCreativeHotbarEntry(editor.interaction.hotbar);
      const bool gradeEligible =
          targetsRenderedDocument &&
          held.kind == creative::CreativeHeldItemKind::TerrainGrade &&
          editor.terrain.grade.active && !modalBlocksWorldActions &&
          capturePath.empty();
      if (gradeEligible) {
        static_cast<void>(refreshCreativeEditorTerrainGradePreview(
            renderDocument, editor));
        if (creativeEditorTerrainGradePreviewMatches(editor.terrain.grade,
                                                      renderDocument)) {
          operationPreview = &editor.terrain.grade.operationPreview;
        } else {
          operationPreview = nullptr;
        }
      }

      if (operationPreview == nullptr) {
        invalidateCreativeEditorGeneratedTerrainPreview(
            terrainOperationPreviewCache);
        return;
      }
      static_cast<void>(refreshCreativeEditorGeneratedTerrainPreview(
          terrainOperationPreviewCache, sceneCache, renderDocument,
          operationPreview->heightField,
          operationPreview->receipt.replay.heightHash,
          operationPreview->materialField,
          operationPreview->receipt.replay.materialHash,
          &bootstrapData.staticMeshAssetCatalog));
    };
    refreshTerrainOperationScenePreview();
    StandaloneRoomBakePreviewScene* selectedPreview = &sceneCache.preview;
    if (terrainOperationPreviewCache.valid) {
      selectedPreview = &terrainOperationPreviewCache.preview;
    }
    if (volumeScenePreviewCache.valid) {
      selectedPreview = &volumeScenePreviewCache.scene.preview;
    }
    const creative::CreativeTerrainSurfacePlan* terrainContourSurface =
        &sceneCache.composedTerrainSurface;
    std::uint64_t terrainContourSurfaceKey =
        sceneCache.terrainSurfaceBuildCount;
    if (terrainOperationPreviewCache.valid &&
        selectedPreview == &terrainOperationPreviewCache.preview) {
      terrainContourSurface = &terrainOperationPreviewCache.composedSurface;
      terrainContourSurfaceKey = terrainOperationPreviewCache.heightHash;
    }
    SceneProjectionResult& scene = selectedPreview->scene;
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

    // World input mutates the CreativeDocument after the frame's initial scene
    // bake. Refresh only changed frames so an accepted placement is submitted
    // immediately rather than leaving the renderer on the pre-click snapshot.
    static_cast<void>(refreshCreativeEditorSceneCache(
        sceneCache, renderDocument,
        &bootstrapData.staticMeshAssetCatalog));
    refreshVolumeScenePreview();
    refreshTerrainOperationScenePreview();
    selectedPreview = volumeScenePreviewCache.valid
                          ? &volumeScenePreviewCache.scene.preview
                          : terrainOperationPreviewCache.valid
                                ? &terrainOperationPreviewCache.preview
                                : &sceneCache.preview;
    frame.projections.scene = &selectedPreview->scene;
    frame.clock.sourceTick = selectedPreview->scene.sourceTick;

    // ---- RESOLVE THE SELECTION (generic) -----------------------------------
    // Everything downstream — the yellow box, gizmo, and dimension label — keys
    // off the CURRENTLY SELECTED object id, looked up via the same
    // findObject the inspector uses. No hardcoded crate id, no kind check. When
    // nothing is selected we draw no gizmo/box and skip Move.
    const CreativeEditorSelectionFrame selection =
        resolveCreativeEditorSelectionFrame(activeAppState.facade);
    std::uint64_t playerSpawnRoomBakeRevision = sceneCache.refreshCount;
    if (selectedPreview == &terrainOperationPreviewCache.preview) {
      playerSpawnRoomBakeRevision = terrainOperationPreviewCache.refreshCount;
    } else if (selectedPreview == &volumeScenePreviewCache.scene.preview) {
      playerSpawnRoomBakeRevision =
          volumeScenePreviewCache.operationRefreshCount;
    }
    static_cast<void>(
        iggy3d_creative_app::refreshCreativePlayerSpawnPreviewCache(
            playerSpawnPreviewCache, activeAppState.facade.document(),
            &selectedPreview->roomBake, selection.selected,
            playerSpawnRoomBakeRevision));
    static_cast<void>(iggy3d_creative_app::syncCreativeMovingPlatformPreview(
        editor.movingPlatformPreview,
        activeAppState.facade.document().id(), selection.selected));
    static_cast<void>(
        iggy3d_creative_app::advanceCreativeMovingPlatformPreview(
            editor.movingPlatformPreview,
            frame.clock.presentationDeltaSeconds));

    // ---- GIZMO GEOMETRY -----------------------------------------------------
    // Build the 3 axis shafts at the selected object's center C = (min+max)/2.
    // Gizmo wireframe lines are appended to the yellow selection-box lines.
    const CreativeEditorGizmoFrame gizmoFrame = buildCreativeEditorGizmoFrame(
        selection, editor.interaction.movingPlatformPathEdit, frame.camera,
        extent.width, extent.height, kGizmoAxisLength, contentViewport,
        &editor.transform);

    logCreativeEditorPathHandleCaptureFrame(
        editor.captureScript, !capturePath.empty(), gizmoFrame);

    CreativeEditorOverlayFrame overlayFrame;
    buildAndAttachCreativeEditorOverlayFrame(
        {activeAppState,
         editor,
         selection,
         gizmoFrame,
         frame,
         wireProjReq,
         extent.width,
         extent.height,
         kGizmoThickness,
         !capturePath.empty(),
         frameInput.inputFrame.context,
         frameInput.activeControlDevice,
         &bootstrapData.staticMeshAssetCatalog,
         &sceneCache.placementClearance,
         &renderDocument,
         terrainContourSurface,
         terrainContourSurfaceKey,
         &playerSpawnPreviewCache.geometry},
        overlayFrame);

    iggy3d_creative_app::endCreativeEditorDesktopFrame(editor.desktopUi);
    StandaloneRoomBakePreviewScene& roomBakePreview = *selectedPreview;
    if (submitCreativeEditorFrame({
            *backend,
            frame,
            activeAppState,
            editor,
            selection,
            overlayFrame,
            roomBakePreview,
            maxFrames})) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
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
