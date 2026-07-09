// iggy3d_creative is the standalone creative-editor lab. It owns a small Vulkan
// app shell and drives the shared creative kernel directly: CreativeDocument,
// Facade tools/mutations, descriptor-driven placement, UI projection,
// wireframe/debug output, RoomBake preview, and save/open round-trip proof.
//
// This file remains the app integration surface. Keep authoring truth in the
// creative kernel and keep deterministic proof state in the extracted helpers:
// EditorBootstrap owns renderer/bootstrap data, EditorCapture owns the fixed
// capture schedule, and the Editor* helpers own picking, placement, gizmo/path
// editing, RoomBake preview, persistence proof, and app-local snapshot undo.
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

#include "app/iggy3d/window/FramePresenter.hpp"
#include "app/platform/SdlWindow.hpp"
#include "core/math/Vec3.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/FrameInput.hpp"
#include "render/vulkan/VulkanBackend.hpp"

#include "EditorCapture.hpp"
#include "EditorFrame.hpp"
#include "EditorGizmo.hpp"
#include "EditorBootstrap.hpp"
#include "EditorState.hpp"
#include "EditorPlacement.hpp"
#include "EditorPreviewFrame.hpp"

namespace {

using namespace iggy3d;
using iggy3d_creative_app::buildCreativeEditorGizmoFrame;
using iggy3d_creative_app::buildCreativeEditorPickFrame;
using iggy3d_creative_app::buildStandaloneRoomBakePreviewScene;
using iggy3d_creative_app::captureFrameToPng;
using iggy3d_creative_app::createCreativeRenderer;
using iggy3d_creative_app::CreativeEditorBootstrapData;
using iggy3d_creative_app::CreativeEditorState;
using iggy3d_creative_app::resolveCreativeEditorAimCell;
using iggy3d_creative_app::applyCreativeEditorClickSelection;
using iggy3d_creative_app::applyCreativeEditorCommandInput;
using iggy3d_creative_app::applyCreativeEditorPlacementInput;
using iggy3d_creative_app::beginCreativeEditorFrameInput;
using iggy3d_creative_app::CreativeEditorFrameInputResult;
using iggy3d_creative_app::CreativeEditorGizmoFrame;
using iggy3d_creative_app::CreativeEditorPickFrame;
using iggy3d_creative_app::CreativeEditorSelectionFrame;
using iggy3d_creative_app::CreativeEditorOverlayFrame;
using iggy3d_creative_app::firstBrushKind;
using iggy3d_creative_app::logCreativeEditorPathHandleCaptureFrame;
using iggy3d_creative_app::processCreativeEditorMoveFrame;
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
  const creative::CreativeSpatialProjectionRequest& wireProjReq =
      bootstrapData.wireProjectionRequest;
  const float kGizmoAxisLength = bootstrapData.gizmoAxisLengthMeters;
  const float kGizmoThickness = bootstrapData.gizmoThicknessMeters;
  const float kGizmoHandleThresholdPx =
      bootstrapData.gizmoHandleThresholdPixels;

  while (window.isOpen()) {
    const CreativeEditorFrameInputResult frameInput =
        beginCreativeEditorFrameInput(window, *backend, editor);
    if (!frameInput.keepRunning) {
      break;
    }
    if (frameInput.skipFrame) {
      continue;
    }
    const SdlDrawableExtent extent = frameInput.extent;
    const bool* keys = frameInput.keyboardState;

    applyCreativeEditorCommandInput(
        keys, !capturePath.empty(), appState, editor, saveRoot, saveId);

    // SCENE (local, must outlive submitFrame): bake supported room geometry
    // through the same CreativeDocument -> RoomAsset adapter that gameplay will
    // eventually consume, then project that RoomAsset through the runtime scene
    // path. Standalone-only editor proxies remain only for objects that RoomBake
    // did not emit as static geometry, such as Point anchors and Path routes.
    StandaloneRoomBakePreviewScene roomBakePreview =
        buildStandaloneRoomBakePreviewScene(appState.facade.document(),
                                            gridSnapshot);
    SceneProjectionResult& scene = roomBakePreview.scene;
    DebugProjectionResult debug{};

    // FRAME (non-const so we can attach UI + wireframe + label below). This
    // gives frame.camera.clipFromWorld (world -> NDC) for click + label maths.
    FrameInput frame = makeProductVulkanFrame(
        scene, debug, editor.frameIndex++, extent.width, extent.height,
        editor.yawDegrees, editor.pitchDegrees,
        /*cameraAnchorOverrideAvailable=*/true, editor.flyPos);

    const Vec3 aimCellCenter =
        resolveCreativeEditorAimCell(frame.camera, editor.placeCellSize);

    // ---- CLICK-TO-SELECT (generic over ALL objects) ------------------------
    // Scan every visible object's visual bounds with one world-space ray. The
    // nearest ray-entry distance wins, so overlapping projected boxes select the
    // closest surface instead of the object with the nearest projected center.
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

    applyCreativeEditorClickSelection(window,
                                      appState,
                                      frame.camera,
                                      extent.width,
                                      extent.height,
                                      pickFrame,
                                      editor,
                                      !capturePath.empty());

    applyCreativeEditorPlacementInput(
        window, appState, editor, aimCellCenter, !capturePath.empty());

    runCreativeEditorCaptureScenarioFrame(
        appState, editor, saveRoot, saveId, !capturePath.empty());

    // ---- RESOLVE THE SELECTION (generic) -----------------------------------
    // Everything downstream — the yellow box, the gizmo, the dimension label, and
    // the Move — keys off the CURRENTLY SELECTED object id, looked up via the same
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
    processCreativeEditorMoveFrame({
        window,
        appState,
        editor,
        selection,
        gizmoFrame,
        frame.camera,
        extent.width,
        extent.height,
        kGizmoAxisLength,
        kGizmoHandleThresholdPx,
        !capturePath.empty()});

    CreativeEditorOverlayFrame overlayFrame;
    buildAndAttachCreativeEditorOverlayFrame(
        {appState,
         editor,
         selection,
         gizmoFrame,
         frame,
         wireProjReq,
         aimCellCenter,
         extent.width,
         extent.height,
         kGizmoThickness},
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

  bool captureOk = true;
  if (!capturePath.empty()) {
    captureOk = captureFrameToPng(*backend, capturePath);
  }
  backend->waitIdle();
  backend->shutdown();
  return captureOk ? 0 : 1;
}
