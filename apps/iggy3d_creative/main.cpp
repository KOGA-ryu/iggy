// iggy3d_creative is the standalone creative-editor lab. It owns a small Vulkan
// app shell and drives the shared creative kernel directly: CreativeDocument,
// Facade tools/mutations, descriptor-driven placement, UI projection,
// wireframe/debug output, RoomBake preview, and save/open round-trip proof.
//
// This file remains the app integration surface. Keep authoring truth in the
// creative kernel and keep deterministic proof state in the extracted helpers:
// EditorCapture.hpp owns the fixed capture schedule, while the
// Standalone* helpers own renderer bootstrap, picking, placement, gizmo/path
// editing, RoomBake preview, persistence proof, and app-local snapshot undo.
//
// Rendered room geometry for bake-supported objects comes from RoomBake and the
// product scene projection. Standalone-only previews stay app-local: point
// markers are visual metadata for baked anchors, path/handle overlays have no
// runtime room geometry yet, and the green placement ghost is an editor affordance.
// The app may contain projection/hit-test glue, but object kind policy should
// continue to come from descriptors and shared kernel systems.

#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <SDL3/SDL.h>

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Core.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/camera/Fly.hpp"
#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/document/DocumentMutation.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/spatial/SpatialProjection.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"
#include "app/iggy3d/creative/world/WorldService.hpp"
#include "app/iggy3d/gameplay/ProjectionRefresh.hpp"
#include "app/iggy3d/map_maker/Grid.hpp"
#include "app/platform/SdlWindow.hpp"
#include "core/math/Mat4.hpp"
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
#include "EditorFrustumCull.hpp"
#include "EditorPathEditing.hpp"
#include "EditorPicking.hpp"
#include "EditorPersistence.hpp"
#include "EditorPreviewProxies.hpp"
#include "EditorPreviewFrame.hpp"
#include "EditorEdits.hpp"

namespace {

using namespace iggy3d;
using iggy3d_creative_app::StandaloneCaptureScript;
using iggy3d_creative_app::StandaloneUndoStack;
using iggy3d_creative_app::BrushFootprint;
using iggy3d_creative_app::brushFootprintForDescriptor;
using iggy3d_creative_app::buildBrushPaletteFromDescriptors;
using iggy3d_creative_app::buildCreativeEditorGizmoFrame;
using iggy3d_creative_app::buildCreativeEditorPickFrame;
using iggy3d_creative_app::buildStandaloneRoomBakePreviewScene;
using iggy3d_creative_app::captureFrameToPng;
using iggy3d_creative_app::createCreativeRenderer;
using iggy3d_creative_app::resolveCreativeEditorAimCell;
using iggy3d_creative_app::resolveCreativeEditorGroundPoint;
using iggy3d_creative_app::applyCreativeEditorClickSelection;
using iggy3d_creative_app::applyCreativeEditorCommandInput;
using iggy3d_creative_app::applyCreativeEditorPlacementInput;
using iggy3d_creative_app::beginCreativeEditorFrameInput;
using iggy3d_creative_app::CreativeEditorState;
using iggy3d_creative_app::CreativeEditorFrameInputResult;
using iggy3d_creative_app::CreativeEditorGizmoFrame;
using iggy3d_creative_app::CreativeEditorPickFrame;
using iggy3d_creative_app::CreativeEditorSelectionFrame;
using iggy3d_creative_app::CreativeEditorOverlayFrame;
using iggy3d_creative_app::firstBrushKind;
using iggy3d_creative_app::logCreativeEditorPathHandleCaptureFrame;
using iggy3d_creative_app::processCreativeEditorMoveFrame;
using iggy3d_creative_app::logCreativeEditorWorldPickProofFrame;
using iggy3d_creative_app::resolveCreativeEditorSelectionFrame;
using iggy3d_creative_app::runCreativeEditorCaptureScenarioFrame;
using iggy3d_creative_app::ScreenPoint;
using iggy3d_creative_app::StandaloneRoomBakePreviewScene;
using iggy3d_creative_app::logStandaloneRoomBakeFinal;

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

  CreativeEditorState editor;
  // Fly camera state. Start pulled back and up, looking at the origin.
  editor.flyConfig.enabled = true;
  editor.flyConfig.speedMetersPerSecond = 8.0F;
  editor.flyConfig.sprintMultiplier = 3.0F;
  editor.flyConfig.inputStepSeconds = 1.0F / 60.0F;

  // Build the ground grid ONCE: a single layer at Y=0 (extentYMeters=0 so it
  // does not stack ~9 layers), anchored at the origin.
  ProductMapMakerGridConfig gridConfig;
  gridConfig.enabled = true;
  gridConfig.pitchMeters = 1.0F;
  gridConfig.majorStepMeters = 5.0F;
  gridConfig.extentXMeters = 40.0F;
  gridConfig.extentYMeters = 1.0F;  // Must be > 0 (config validity); the
                                    // ground layer is filtered in
                                    // standalone preview scene build.
  gridConfig.extentZMeters = 40.0F;
  gridConfig.planeY = 0.0F;
  gridConfig.anchorWorld = Vec3{0.0F, 0.0F, 0.0F};
  const ProductMapMakerGridSnapshot gridSnapshot =
      buildProductMapMakerGridSnapshot(gridConfig);
  SDL_Log("iggy3d_creative: grid visible=%d layers=%llu dots=%llu",
          gridSnapshot.visible ? 1 : 0,
          static_cast<unsigned long long>(gridSnapshot.layerCount),
          static_cast<unsigned long long>(gridSnapshot.dotCount));

  // ---- Seed initial CreativeDocument objects -----------------------------
  // A Floor tile sitting on Y=0 and a Crate resting on top of it. Both are
  // authored through the SAME generic createDocumentObject path; Floor needs no
  // new kernel work because CreativeObjectKind::Floor already ships a descriptor.
  creative::CreativeAppState appState;
  {
    creative::CreativeDocument doc =
        creative::CreativeDocument::create("FloorAndCrateWorld");
    (void)doc.assignId(1);
    const creative::CreativeFacadeDocumentInstallReceipt installReceipt =
        appState.facade.installDocument(std::move(doc));
    SDL_Log("iggy3d_creative: install document accepted=%d",
            installReceipt.accepted ? 1 : 0);
  }

  // FLOOR 1: a 4 x 0.25 x 4 walkable tile whose top sits at Y=0.25 with its slab
  // straddling Y=0. Same authoring request struct as the crate — only kind and
  // extents differ; no per-kind create path.
  creative::CreativeDocumentCreateRequest floorRequest;
  floorRequest.kind = creative::CreativeObjectKind::Floor;
  floorRequest.name = "Floor 1";
  floorRequest.transform.position = {0.0, 0.125, 0.0};
  floorRequest.hasTransformOverride = true;
  floorRequest.bounds = {{-2.0, 0.0, -2.0}, {2.0, 0.25, 2.0}};
  floorRequest.hasBoundsOverride = true;
  floorRequest.visible = true;
  floorRequest.hasVisibleOverride = true;
  floorRequest.locked = false;
  floorRequest.hasLockedOverride = true;
  const creative::CreativeDocumentCreateReceipt floorReceipt =
      appState.facade.createDocumentObject(floorRequest);
  const creative::CreativeObjectId floorObjectId = floorReceipt.objectId;
  SDL_Log("iggy3d_creative: floor create accepted=%d objectId=%llu kind='%s'",
          floorReceipt.accepted ? 1 : 0,
          static_cast<unsigned long long>(floorObjectId),
          std::string(creative::toString(floorReceipt.objectKind)).c_str());

  // CRATE 1: a 1 m cube resting ON the floor (bottom at Y=0.25, top at Y=1.25),
  // offset in Z so it does not eclipse the floor tile's center from the camera.
  creative::CreativeDocumentCreateRequest crateRequest;
  crateRequest.kind = creative::CreativeObjectKind::Crate;
  crateRequest.name = "Crate 1";
  crateRequest.transform.position = {0.0, 0.375, 0.0};
  crateRequest.hasTransformOverride = true;
  crateRequest.bounds = {{-0.5, 0.25, -0.5}, {0.5, 1.25, 0.5}};
  crateRequest.hasBoundsOverride = true;
  crateRequest.visible = true;
  crateRequest.hasVisibleOverride = true;
  crateRequest.locked = false;
  crateRequest.hasLockedOverride = true;
  const creative::CreativeDocumentCreateReceipt crateReceipt =
      appState.facade.createDocumentObject(crateRequest);
  const creative::CreativeObjectId crateObjectId = crateReceipt.objectId;
  SDL_Log("iggy3d_creative: crate create accepted=%d objectId=%llu kind='%s'",
          crateReceipt.accepted ? 1 : 0,
          static_cast<unsigned long long>(crateObjectId),
          std::string(creative::toString(crateReceipt.objectKind)).c_str());
  (void)crateObjectId;  // Retained for the log; tooling keys off the SELECTION.

  // ---- SAVE LOCATION ------------------------------------------------------
  // A single fixed save slot for the standalone app: <HOME>/.iggy3d/
  // creative_standalone with saveId "scene". Created up front so the kernel's
  // creative-save always has a writable root. One slot is enough for the
  // standalone proof.
  std::filesystem::path saveRoot;
  if (const char* home = std::getenv("HOME"); home != nullptr) {
    saveRoot = std::filesystem::path{home} / ".iggy3d" / "creative_standalone";
  } else {
    saveRoot = std::filesystem::path{".iggy3d"} / "creative_standalone";
  }
  const std::string saveId = "scene";
  {
    std::error_code ec;
    std::filesystem::create_directories(saveRoot, ec);
    SDL_Log("iggy3d_creative: saveRoot='%s' saveId='%s' created=%d",
            saveRoot.generic_string().c_str(), saveId.c_str(),
            ec ? 0 : 1);
  }

  // The wireframe projection request: a grid big enough to hold the origin
  // crate (world Y 0..1 fits in height=8; XZ clamp handles the negative corner).
  creative::CreativeSpatialProjectionRequest wireProjReq;
  wireProjReq.gridSize = {80, 8, 80};
  wireProjReq.cellSize = 1.0;
  wireProjReq.clampToGrid = true;
  wireProjReq.includeAuthoringOnly = false;

  // ---- MOVE state --------------------------------------------------------
  // Interactive: edge-triggered key latches for '1' Select / '2' Move so a held
  // key switches the tool exactly once. Interactive drag latch tracks a left
  // button held while the Move tool is active.
  // --capture: run the generic Move lifecycle across a few frames, and log the
  // crate placement BEFORE the commit and AFTER the release exactly once.
  // ---- GIZMO state -------------------------------------------------------
  // Axis shaft length (m) and wireframe thickness (m). Kept short so the shafts
  // read as handles, not room-scale rays; thickness ~5 cm per the plan.
  constexpr float kGizmoAxisLength = 1.5F;
  constexpr float kGizmoThickness = 0.05F;
  // Handle hit-test threshold (px): a click within this pixel distance of a
  // projected shaft grabs that axis; the nearest axis within range wins.
  constexpr float kGizmoHandleThresholdPx = 35.0F;
  // Interactive: which axis is currently grabbed (None = not dragging a handle),
  // the object's start corner anchor S captured at grab time, and the pixel/world
  // frame captured at grab so a cursor drag maps to a world offset along the axis.
  // --capture: log the grabbed axis exactly once.

  // ---- PLACE state -------------------------------------------------------
  // placeMode is an APP-level mode (not a kernel Tool) toggled by '3'. When on,
  // the click drops a NEW object at the aimed cell instead of running the
  // select/move hit-test. '1'/'2' leave place mode and set the kernel tool.
  // placeBrush is the current descriptor-backed brush kind. The grid pitch
  // (1 m) is the placement cell size for snapping.
  editor.brushPalette = buildBrushPaletteFromDescriptors();
  editor.placeBrush = firstBrushKind(editor.brushPalette);
  SDL_Log("iggy3d_creative: brush palette slots=%llu first='%s'",
          static_cast<unsigned long long>(editor.brushPalette.size()),
          std::string(creative::toString(editor.placeBrush)).c_str());
  editor.placeCellSize = static_cast<double>(gridConfig.pitchMeters);
  // --capture: in Place mode we start ON so the proof frames can drop objects.
  if (!capturePath.empty()) {
    editor.placeMode = true;
    editor.placeBrush = firstBrushKind(editor.brushPalette);
  }
  // ---- SAVE / LOAD state ------------------------------------------------
  // Interactive: edge latches for F5 (save), F6 (new/clear), F9 (load).
  // --capture round-trip proof: prove create/delete/move undo, add Point and
  // Line + Path markers, undo their moves, then SAVE/CLEAR/LOAD the
  // eight-object scene. The schedule, flags, ids, and snapshots live in the
  // capture script helper; main only executes the current frame's authored step.

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
    const creative::Id selectedId = selection.selectedId;
    const creative::CreativeObject* selected = selection.selected;
    const bool hasSelection = selection.hasSelection;

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

    const iggy3d_creative_app::StandaloneFrustumCullResult frustumCull =
        iggy3d_creative_app::cullStandaloneSceneRoomMeshesByFrustum(
            *frame.projections.scene, frame.camera.clipFromWorld);
    frame.projections.scene = &frustumCull.scene;

    const RenderSubmitResult submit = backend->submitFrame(frame);
    if (!editor.loggedSelection) {
      editor.loggedSelection = true;
      SDL_Log("iggy3d_creative: frame %llu submit outcome=%d reason='%s' "
              "meshes=%zu frustumInputMeshes=%zu frustumKeptMeshes=%zu "
              "frustumCulledMeshes=%zu frustumConservativeMeshes=%zu "
              "selectedTarget=%u hasSelection=%d selBoxLines=%zu "
              "pointMarkerLines=%zu lineMarkerLines=%zu pathHandleLines=%zu "
              "gizmoLines=%zu combinedWireLines=%zu uiRects=%zu glyphs=%zu",
              static_cast<unsigned long long>(editor.frameIndex),
              static_cast<int>(submit.outcome),
              std::string(submit.reason.code).c_str(),
              frustumCull.scene.room.meshes.size(),
              frustumCull.receipt.inputRoomMeshCount,
              frustumCull.receipt.keptRoomMeshCount,
              frustumCull.receipt.culledRoomMeshCount,
              frustumCull.receipt.conservativelyKeptMeshCount, selectedId,
              hasSelection ? 1 : 0,
              overlayFrame.documentWireLineCount,
              overlayFrame.pointMarkerEdgeCount,
              overlayFrame.lineMarkerEdgeCount,
              overlayFrame.pathPointHandleEdgeCount,
              overlayFrame.combinedWireLines.size() -
                  overlayFrame.documentWireLineCount -
                  overlayFrame.pointMarkerEdgeCount -
                  overlayFrame.lineMarkerEdgeCount -
                  overlayFrame.pathPointHandleEdgeCount,
              overlayFrame.combinedWireLines.size(),
              overlayFrame.uiRects.size(), overlayFrame.glyphs.size());
    }

    if (maxFrames != 0U && editor.frameIndex >= maxFrames) {
      logStandaloneRoomBakeFinal(roomBakePreview);
      // Name the SELECTED object + kind so the capture is self-documenting; the
      // capture proof expects this target to be the FLOOR.
      const char* selKind =
          hasSelection
              ? creative::toString(selected->kind).data()
              : "<none>";
      SDL_Log("iggy3d_creative: FINAL frame %llu submit outcome=%d reason='%s' "
              "selectedTarget=%u selectedKind='%s' hasSelection=%d selBoxLines=%zu "
              "pointMarkerLines=%zu lineMarkerLines=%zu pathHandleLines=%zu "
              "gizmoLines=%zu combinedWireLines=%zu placeMode=%d brush='%s' "
              "ghostEdges=%zu placed=%llu objectCount=%llu "
              "frustumInputMeshes=%zu frustumKeptMeshes=%zu "
              "frustumCulledMeshes=%zu frustumConservativeMeshes=%zu",
              static_cast<unsigned long long>(editor.frameIndex),
              static_cast<int>(submit.outcome),
              std::string(submit.reason.code).c_str(), selectedId, selKind,
              hasSelection ? 1 : 0, overlayFrame.documentWireLineCount,
              overlayFrame.pointMarkerEdgeCount,
              overlayFrame.lineMarkerEdgeCount,
              overlayFrame.pathPointHandleEdgeCount,
              overlayFrame.combinedWireLines.size() -
                  overlayFrame.documentWireLineCount -
                  overlayFrame.pointMarkerEdgeCount -
                  overlayFrame.lineMarkerEdgeCount -
                  overlayFrame.pathPointHandleEdgeCount,
              overlayFrame.combinedWireLines.size(), editor.placeMode ? 1 : 0,
              std::string(creative::toString(editor.placeBrush)).c_str(),
              overlayFrame.ghostEdgeCount,
              static_cast<unsigned long long>(editor.placedCount),
              static_cast<unsigned long long>(
                  appState.facade.document().objectCount()),
              frustumCull.receipt.inputRoomMeshCount,
              frustumCull.receipt.keptRoomMeshCount,
              frustumCull.receipt.culledRoomMeshCount,
              frustumCull.receipt.conservativelyKeptMeshCount);
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
