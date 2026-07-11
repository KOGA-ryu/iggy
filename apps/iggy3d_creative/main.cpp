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

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>

#include <SDL3/SDL.h>

#include "app/iggy3d/creative/render/CreativeSceneFrame.hpp"
#include "app/platform/SdlWindow.hpp"
#include "core/math/Vec3.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/FrameInput.hpp"
#include "render/vulkan/VulkanBackend.hpp"

#include "EditorCapture.hpp"
#include "EditorCatalog.hpp"
#include "EditorControls.hpp"
#include "EditorFrame.hpp"
#include "EditorGamepad.hpp"
#include "EditorGizmo.hpp"
#include "EditorBootstrap.hpp"
#include "EditorInteraction.hpp"
#include "EditorState.hpp"
#include "EditorToolOptions.hpp"
#include "EditorPlacement.hpp"
#include "EditorPreviewFrame.hpp"
#include "EditorTransform.hpp"

namespace {

using namespace iggy3d;
using iggy3d_creative_app::buildCreativeEditorGizmoFrame;
using iggy3d_creative_app::buildCreativeEditorPickFrame;
using iggy3d_creative_app::buildStandaloneRoomBakePreviewScene;
using iggy3d_creative_app::captureFrameToPng;
using iggy3d_creative_app::createCreativeRenderer;
using iggy3d_creative_app::CreativeEditorBootstrapData;
using iggy3d_creative_app::CreativeEditorGamepad;
using iggy3d_creative_app::CreativeEditorState;
using iggy3d_creative_app::applyCreativeEditorCommandInput;
using iggy3d_creative_app::beginCreativeEditorFrameInput;
using iggy3d_creative_app::cancelCreativeEditorSelectionTransformPreview;
using iggy3d_creative_app::processCreativeEditorTransformFrame;
using iggy3d_creative_app::CreativeEditorFrameInputResult;
using iggy3d_creative_app::CreativeEditorGizmoFrame;
using iggy3d_creative_app::CreativeEditorPickFrame;
using iggy3d_creative_app::CreativeEditorSelectionFrame;
using iggy3d_creative_app::CreativeEditorOverlayFrame;
using iggy3d_creative_app::firstBrushKind;
using iggy3d_creative_app::logCreativeEditorPathHandleCaptureFrame;
using iggy3d_creative_app::processCreativeEditorCatalogFrame;
using iggy3d_creative_app::processCreativeEditorControlsFrame;
using iggy3d_creative_app::processCreativeEditorToolOptionsFrame;
using iggy3d_creative_app::processCreativeEditorWorldInteractionFrame;
using iggy3d_creative_app::submitCreativeEditorFrame;
using iggy3d_creative_app::logCreativeEditorWorldPickProofFrame;
using iggy3d_creative_app::resolveCreativeEditorSelectionFrame;
using iggy3d_creative_app::runCreativeEditorCaptureScenarioFrame;
using iggy3d_creative_app::StandaloneRoomBakePreviewScene;
using iggy3d_creative_app::initializeCreativeEditorBootstrapData;

}  // namespace

int main(int argc, char** argv) {
  // Optional flags:
  //   --frames N        auto-exit after N presented frames (scriptable run)
  //   --capture <path>  render a few frames, write <path>.png, then exit
  std::uint64_t maxFrames = 0;  // 0 = run until window close.
  std::string capturePath;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--frames" && i + 1 < argc) {
      maxFrames = std::strtoull(argv[++i], nullptr, 10);
    } else if (arg == "--capture" && i + 1 < argc) {
      capturePath = argv[++i];
    }
  }
  // In capture mode, render a handful of frames to let the swapchain settle,
  // then grab the last one.
  if (!capturePath.empty() && maxFrames == 0U) {
    maxFrames = 8U;
  }

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

  std::unique_ptr<VulkanBackend> backend = createCreativeRenderer(window);
  if (backend == nullptr ||
      backend->lifecycleState() != RendererLifecycleState::Ready) {
    SDL_Log("iggy3d_creative: renderer not ready (lifecycle=%d)",
            backend == nullptr ? -1
                               : static_cast<int>(backend->lifecycleState()));
    return 1;
  }

  // Relative mouse mode for a free-look fly camera (interactive only — don't
  // grab the mouse during a scripted --capture run).
  if (capturePath.empty()) {
    window.setRelativeMouseMode(true);
  }

  CreativeEditorBootstrapData bootstrapData;
  initializeCreativeEditorBootstrapData(bootstrapData, !capturePath.empty());
  CreativeEditorState& editor = bootstrapData.editor;
  const ProductMapMakerGridSnapshot& gridSnapshot =
      bootstrapData.gridSnapshot;
  creative::CreativeAppState& appState = bootstrapData.appState;
  const creative::CreativeObjectId& floorObjectId =
      bootstrapData.floorObjectId;
  const std::filesystem::path& saveRoot = bootstrapData.saveRoot;
  const std::string& saveId = bootstrapData.saveId;
  const std::filesystem::path controlsPath =
      saveRoot / "creative_controls_v1.cfg";
  static_cast<void>(iggy3d_creative_app::loadCreativeEditorControlProfile(
      editor.controlProfile, controlsPath));
  const creative::CreativeSpatialProjectionRequest& wireProjReq =
      bootstrapData.wireProjectionRequest;
  const float kGizmoAxisLength = bootstrapData.gizmoAxisLengthMeters;
  const float kGizmoThickness = bootstrapData.gizmoThicknessMeters;
  CreativeEditorGamepad gamepad;
  iggy3d_creative_app::CreativeEditorSceneCache sceneCache;

  while (window.isOpen()) {
    const CreativeEditorFrameInputResult frameInput =
        beginCreativeEditorFrameInput(
            window, *backend, gamepad, editor, !capturePath.empty());
    if (!frameInput.keepRunning) {
      break;
    }
    if (frameInput.skipFrame) {
      if (!frameInput.windowFocused) {
        finalizeCreativeMaterialStroke(
            appState, editor, "creative_material_stroke_focus_lost");
        if (cancelCreativeEditorSelectionTransformPreview(
                editor.transform, "selection_transform_focus_lost")) {
          static_cast<void>(window.setRelativeMouseMode(true));
        }
      }
      continue;
    }
    const SdlDrawableExtent extent = frameInput.extent;
    if (!frameInput.windowFocused &&
        cancelCreativeEditorSelectionTransformPreview(
            editor.transform, "selection_transform_focus_lost")) {
      static_cast<void>(window.setRelativeMouseMode(true));
    }
    const iggy3d_creative_app::CreativeEditorControlsFrameResult controlsFrame =
        processCreativeEditorControlsFrame(
            {window,
             editor,
             frameInput.routedInput,
             frameInput.inputFrame,
             controlsPath,
             frameInput.monotonicTimeNanoseconds,
             extent.width,
             extent.height});
    const iggy3d_creative_app::CreativeEditorTransformFrameResult
        transformFrame = processCreativeEditorTransformFrame(
            {window,
             appState,
             editor,
             frameInput.routedInput,
             frameInput.toolWheelDirectionX,
             frameInput.toolWheelDirectionY,
             frameInput.transformNudgeWheelSteps,
             frameInput.transformFineNudge,
             extent.width,
             extent.height});
    const iggy3d_creative_app::CreativeEditorCatalogFrameResult catalogFrame =
        processCreativeEditorCatalogFrame(
            {window,
             appState,
             editor,
             frameInput.routedInput,
             frameInput.worldActions,
             frameInput.toolWheelDirectionX,
             frameInput.toolWheelDirectionY,
             extent.width,
             extent.height});
    const iggy3d_creative_app::CreativeEditorToolOptionsFrameResult
        toolOptionsFrame = processCreativeEditorToolOptionsFrame(
            {window,
             editor,
             frameInput.routedInput,
             catalogFrame.openToolOptionsRequested,
             catalogFrame.toolOptionsHeldItem,
             extent.width,
             extent.height});
    const bool modalBlocksWorldActions =
        controlsFrame.blockWorldActions || transformFrame.blockWorldActions ||
        catalogFrame.blockWorldActions || toolOptionsFrame.blockWorldActions;
    if (modalBlocksWorldActions || !frameInput.windowFocused) {
      finalizeCreativeMaterialStroke(
          appState, editor,
          frameInput.windowFocused ? "creative_material_stroke_modal"
                                   : "creative_material_stroke_focus_lost");
    }
    if (!catalogFrame.deferredCommandInput.actionEvents().empty()) {
      applyCreativeEditorCommandInput(catalogFrame.deferredCommandInput,
                                      appState, editor, saveRoot, saveId);
    }
    if (!modalBlocksWorldActions && frameInput.windowFocused) {
      applyCreativeEditorCommandInput(
          frameInput.routedInput, appState, editor, saveRoot, saveId);
    }

    // SCENE (local, must outlive submitFrame): bake supported room geometry
    // through the same CreativeDocument -> RoomAsset adapter that gameplay will
    // eventually consume, then project that RoomAsset through the runtime scene
    // path. Standalone-only editor proxies remain only for objects that RoomBake
    // did not emit as static geometry, such as Point anchors and Path routes.
    static_cast<void>(refreshCreativeEditorSceneCache(
        sceneCache, appState.facade.document(), gridSnapshot));
    StandaloneRoomBakePreviewScene& roomBakePreview = sceneCache.preview;
    SceneProjectionResult& scene = roomBakePreview.scene;
    DebugProjectionResult debug{};

    // FRAME (non-const so we can attach UI + wireframe + label below). This
    // gives frame.camera.clipFromWorld (world -> NDC) for click + label maths.
    FrameInput frame = makeCreativeVulkanFrame(
        scene, debug, editor.frameIndex++, extent.width, extent.height,
        editor.yawDegrees, editor.pitchDegrees,
        /*cameraAnchorOverrideAvailable=*/true, editor.flyPos);

    // Scan every visible object's visual bounds once. Live interaction resolves
    // the center ray from this frame; scripted capture retains its fixed proof ray.
    const CreativeEditorPickFrame pickFrame = buildCreativeEditorPickFrame(
        appState.facade.document(),
        frame.camera,
        extent.width,
        extent.height,
        floorObjectId,
        editor.captureScript,
        !capturePath.empty());
    logCreativeEditorWorldPickProofFrame(appState.facade,
                                         frame.camera,
                                         extent.width,
                                         extent.height,
                                         pickFrame,
                                         floorObjectId,
                                         editor,
                                         !capturePath.empty());

    if (!modalBlocksWorldActions && frameInput.windowFocused) {
      processCreativeEditorWorldInteractionFrame(
          {appState,
           editor,
           frameInput.worldActions,
           frameInput.modifiers,
           frame.camera,
           pickFrame,
           extent.width,
           extent.height,
           frameInput.monotonicTimeNanoseconds,
           !capturePath.empty()});
    }

    runCreativeEditorCaptureScenarioFrame(
        appState, editor, saveRoot, saveId, !capturePath.empty());

    // World input mutates the CreativeDocument after the frame's initial scene
    // bake. Refresh only changed frames so an accepted placement is submitted
    // immediately rather than leaving the renderer on the pre-click snapshot.
    if (refreshCreativeEditorSceneCache(
            sceneCache, appState.facade.document(), gridSnapshot)) {
      frame.projections.scene = &roomBakePreview.scene;
      frame.clock.sourceTick = roomBakePreview.scene.sourceTick;
    }

    // ---- RESOLVE THE SELECTION (generic) -----------------------------------
    // Everything downstream — the yellow box, gizmo, and dimension label — keys
    // off the CURRENTLY SELECTED object id, looked up via the same
    // findObject the inspector uses. No hardcoded crate id, no kind check. When
    // nothing is selected we draw no gizmo/box and skip Move.
    const CreativeEditorSelectionFrame selection =
        resolveCreativeEditorSelectionFrame(appState.facade);

    // ---- GIZMO GEOMETRY -----------------------------------------------------
    // Build the 3 axis shafts at the selected object's center C = (min+max)/2.
    // Each shaft is a single AXIS-ALIGNED world segment (start=C, end=C+dir*L),
    // which is the ONLY geometry the renderer's creativeDebugLineBox will draw
    // (it silently skips any segment moving along more than one world axis). The
    // gizmo wireframe lines are appended to the yellow selection-box lines below.
    const CreativeEditorGizmoFrame gizmoFrame = buildCreativeEditorGizmoFrame(
        selection, frame.camera, extent.width, extent.height, kGizmoAxisLength);

    logCreativeEditorPathHandleCaptureFrame(
        editor.captureScript, !capturePath.empty(), gizmoFrame);

    CreativeEditorOverlayFrame overlayFrame;
    buildAndAttachCreativeEditorOverlayFrame(
        {appState,
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
         frameInput.activeControlDevice},
        overlayFrame);

    if (submitCreativeEditorFrame({
            *backend,
            frame,
            appState,
            editor,
            selection,
            overlayFrame,
            roomBakePreview,
            maxFrames})) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }

  finalizeCreativeMaterialStroke(appState, editor,
                                 "creative_material_stroke_shutdown");

  bool captureOk = true;
  if (!capturePath.empty()) {
    captureOk = captureFrameToPng(*backend, capturePath);
  }
  backend->waitIdle();
  backend->shutdown();
  return captureOk ? 0 : 1;
}
