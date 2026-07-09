// iggy3d_creative is the standalone creative-editor lab. It owns a small Vulkan
// app shell and drives the shared creative kernel directly: CreativeDocument,
// Facade tools/mutations, descriptor-driven placement, UI projection,
// wireframe/debug output, RoomBake preview, and save/open round-trip proof.
//
// This file remains the app integration surface. Keep authoring truth in the
// creative kernel and keep deterministic proof state in the extracted helpers:
// StandaloneCaptureScript.hpp owns the fixed capture schedule, while the
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
#include "app/iggy3d/creative/document/DocumentWireframe.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/render/WireframeDebugLines.hpp"
#include "app/iggy3d/creative/spatial/SpatialProjection.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"
#include "app/iggy3d/creative/ui/UiProjection.hpp"
#include "app/iggy3d/creative/world/WorldService.hpp"
#include "app/iggy3d/gameplay/ProjectionRefresh.hpp"
#include "app/iggy3d/map_maker/Grid.hpp"
#include "app/iggy3d/window/FramePresenter.hpp"
#include "app/platform/SdlWindow.hpp"
#include "core/math/Mat4.hpp"
#include "core/math/Vec3.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "projection/scene/SceneItem.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/FrameInput.hpp"
#include "render/debug/DebugHudText.hpp"
#include "render/vulkan/VulkanBackend.hpp"

#include "CreativeEditorAim.hpp"
#include "CreativeEditorClickSelection.hpp"
#include "CreativeEditorCommandInput.hpp"
#include "CreativeEditorFrameInput.hpp"
#include "CreativeEditorGizmoFrame.hpp"
#include "CreativeEditorPickFrame.hpp"
#include "CreativeEditorPlacementInput.hpp"
#include "CreativeRendererBootstrap.hpp"
#include "CreativeEditorSelection.hpp"
#include "CreativeEditorState.hpp"
#include "StandaloneCaptureScenario.hpp"
#include "StandaloneCaptureScript.hpp"
#include "StandaloneBrushPalette.hpp"
#include "StandaloneDelete.hpp"
#include "StandaloneFrustumCull.hpp"
#include "StandaloneGizmo.hpp"
#include "StandalonePathEditing.hpp"
#include "StandalonePicking.hpp"
#include "StandalonePlacement.hpp"
#include "StandalonePersistenceProof.hpp"
#include "StandalonePreviewProxies.hpp"
#include "StandaloneRoomBakePreview.hpp"
#include "StandaloneUndo.hpp"
#include "StandaloneWireframeBoxEdges.hpp"

namespace {

using namespace iggy3d;
using iggy3d_creative_app::runStandaloneCaptureScenarioStep;
using iggy3d_creative_app::StandaloneCaptureScenarioStepRequest;
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
using iggy3d_creative_app::deleteSelectedObject;
using iggy3d_creative_app::firstBrushKind;
using iggy3d_creative_app::dispatchMoveReleaseWithUndo;
using iggy3d_creative_app::GizmoAxis;
using iggy3d_creative_app::GizmoAxisShaft;
using iggy3d_creative_app::gizmoAxisName;
using iggy3d_creative_app::heldAxisForGrabbedAxis;
using iggy3d_creative_app::initialPathPointsForAnchor;
using iggy3d_creative_app::logCreativeEditorPathHandleCaptureFrame;
using iggy3d_creative_app::logMoveDispatch;
using iggy3d_creative_app::logObjectPlacement;
using iggy3d_creative_app::movePathObjectWithUndo;
using iggy3d_creative_app::movePathPointWithUndo;
using iggy3d_creative_app::pushUndoSnapshot;
using iggy3d_creative_app::appendPathPolylineLines;
using iggy3d_creative_app::lineProxyBounds;
using iggy3d_creative_app::logCreativeEditorWorldPickProofFrame;
using iggy3d_creative_app::ObjectVisualPickBounds;
using iggy3d_creative_app::pathPointHandleBounds;
using iggy3d_creative_app::PathPointHandleHit;
using iggy3d_creative_app::pickPathPointHandle;
using iggy3d_creative_app::pickGizmoAxisFromProjectedShafts;
using iggy3d_creative_app::pointMarkerBounds;
using iggy3d_creative_app::pathPointsSummary;
using iggy3d_creative_app::projectBoxToScreen;
using iggy3d_creative_app::resolveCreativeEditorSelectionFrame;
using iggy3d_creative_app::ScreenPoint;
using iggy3d_creative_app::StandaloneRoomBakePreviewScene;
using iggy3d_creative_app::appendStandaloneWireframeBoxEdges;
using iggy3d_creative_app::toVec3;
using iggy3d_creative_app::VisualBounds;
using iggy3d_creative_app::visualBoundsForObject;
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

    // ---- CAPTURE SCENARIO (--capture) --------------------------------------
    if (!capturePath.empty()) {
      StandaloneCaptureScenarioStepRequest captureStep;
      captureStep.enabled = true;
      captureStep.frameIndex = editor.frameIndex;
      captureStep.appState = &appState;
      captureStep.undoStack = &editor.undoStack;
      captureStep.captureScript = &editor.captureScript;
      captureStep.placeBrush = &editor.placeBrush;
      captureStep.placeMode = &editor.placeMode;
      captureStep.placedCount = &editor.placedCount;
      captureStep.placeCellSize = editor.placeCellSize;
      captureStep.saveRoot = &saveRoot;
      captureStep.saveId = &saveId;
      captureStep.moveHeldAxisForX = heldAxisForGrabbedAxis(GizmoAxis::X);
      captureStep.moveHeldAxisForZ = heldAxisForGrabbedAxis(GizmoAxis::Z);
      captureStep.deleteSelected = [&](std::string_view source) {
        return deleteSelectedObject(appState, source, &editor.undoStack);
      };
      runStandaloneCaptureScenarioStep(captureStep);
    }

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
    const Vec3 selBoxMin = selection.boxMin;
    const Vec3 selBoxMax = selection.boxMax;

    // ---- GIZMO GEOMETRY -----------------------------------------------------
    // Build the 3 axis shafts at the selected object's center C = (min+max)/2.
    // Each shaft is a single AXIS-ALIGNED world segment (start=C, end=C+dir*L),
    // which is the ONLY geometry the renderer's creativeDebugLineBox will draw
    // (it silently skips any segment moving along more than one world axis). The
    // gizmo wireframe lines are appended to the yellow selection-box lines below.
    const CreativeEditorGizmoFrame gizmoFrame = buildCreativeEditorGizmoFrame(
        selection, frame.camera, extent.width, extent.height, kGizmoAxisLength);
    const Vec3 gizmoCenter = gizmoFrame.center;
    const std::array<GizmoAxisShaft, 3>& gizmoShafts = gizmoFrame.shafts;
    const ScreenPoint gizmoCenterScreen = gizmoFrame.centerScreen;
    const std::array<ScreenPoint, 3>& gizmoTipScreen = gizmoFrame.tipScreens;
    const std::vector<PathPointHandleHit>& pathPointHandleHits =
        gizmoFrame.pathPointHandleHits;
    const bool selectedIsPathForHandles =
        gizmoFrame.selectedIsPathForHandles;

    logCreativeEditorPathHandleCaptureFrame(
        editor.captureScript, !capturePath.empty(), gizmoFrame);
    // Start anchor S for an axis-constrained move = the object's corner anchor,
    // exactly as the facade captures it on BeginMove (objectCornerAnchor): for a
    // Crate (hasTransform=true) that is transform.position, NOT bounds.min. Using
    // the SAME anchor the facade holds keeps the pinned axes grid-aligned so they
    // snap to themselves; reading bounds.min instead would desync the pinned axes
    // and let the snap drag a "held" axis off S. Constrained-move worldDestination
    // is built FROM S: grabbed axis carries the dragged value, other two pinned.
    const Vec3 gizmoAnchorS = gizmoFrame.anchorS;

    // ---- MOVE --------------------------------------------------------------
    // Everything below drives the kernel's GENERIC Move: setActiveTool(Move) +
    // the PRESS/MOVE/RELEASE pointer lifecycle through dispatchToolInput. The
    // facade picks the object, snaps the world destination to the grid, and
    // commits ONE Move mutation. NO per-object position math lives here, and the
    // target is ALWAYS the currently selected id — for the capture that is
    // the FLOOR, which rides the identical path the crate did in the earlier proof.
    const creative::CreativeObjectId selectedObjectId =
        static_cast<creative::CreativeObjectId>(selectedId);
    if (!capturePath.empty() && !editor.placeMode) {
      // --capture: after the FLOOR is selected (frame 3), grab the X
      // gizmo handle and run an AXIS-CONSTRAINED Move along +X by 2 m.
      //   frame 5: hit-test the X shaft (grab X) + switch to Move + PRESS
      //   frame 6: PointerMove carrying worldDestination = {S.x+2, S.y, S.z},
      //            moveHeldAxis=Y (PreviewMove) — X follows, Y held, Z pinned
      //   frame 7: RELEASE with the same worldDestination (CommitMove) -> snap
      // The single-axis motion is entirely a product of the destination + held
      // axis; there is NO per-object move math. worldDestination pins Y,Z to the
      // start anchor S so only X (= S.x + 2) can change after the facade snaps.
      const creative::CreativeToolWorldPoint xAxisDestination{
          static_cast<double>(gizmoAnchorS.x) + 2.0,
          static_cast<double>(gizmoAnchorS.y),
          static_cast<double>(gizmoAnchorS.z)};
      if (editor.frameIndex == 5U && hasSelection) {
        // Synthesize a grab of the X handle: click the projected midpoint of the
        // X shaft [screen(C), screen(Xtip)] and confirm the hit-test picks X.
        GizmoAxis grabbed = GizmoAxis::None;
        if (gizmoCenterScreen.valid && gizmoTipScreen[0].valid) {
          const float hx = (gizmoCenterScreen.x + gizmoTipScreen[0].x) * 0.5F;
          const float hy = (gizmoCenterScreen.y + gizmoTipScreen[0].y) * 0.5F;
          grabbed = pickGizmoAxisFromProjectedShafts(
              gizmoShafts,
              gizmoCenterScreen,
              gizmoTipScreen,
              hx,
              hy,
              kGizmoHandleThresholdPx);
        }
        if (!editor.loggedGizmoGrab) {
          SDL_Log("iggy3d_creative: GIZMO grabbed axis=%s (expected X) on "
                  "selected id=%u kind='%s'",
                  gizmoAxisName(grabbed), selectedId,
                  std::string(creative::toString(selected->kind)).c_str());
          editor.loggedGizmoGrab = true;
        }
        const bool ok = appState.facade.setActiveTool(creative::Tool::Move);
        SDL_Log("iggy3d_creative: setActiveTool(Move) accepted=%d", ok ? 1 : 0);
        if (!editor.loggedMoveBefore) {
          logObjectPlacement("BEFORE",
                             appState.facade.findObject(selectedObjectId));
          editor.loggedMoveBefore = true;
        }
        creative::CreativeToolInputPacket press;
        press.kind = creative::CreativeToolInputKind::PointerPress;
        press.pointer.button = creative::CreativeToolPointerButton::Primary;
        press.pointer.target =
            creative::TargetRef{static_cast<creative::Id>(selectedObjectId)};
        const creative::CreativeFacadeToolDispatchReceipt r =
            appState.facade.dispatchToolInput(press);
        logMoveDispatch("PRESS", r);
      } else if (editor.frameIndex == 6U) {
        creative::CreativeToolInputPacket move;
        move.kind = creative::CreativeToolInputKind::PointerMove;
        move.pointer.button = creative::CreativeToolPointerButton::Primary;
        move.pointer.hasWorldDestination = true;
        move.pointer.worldDestination = xAxisDestination;
        move.pointer.moveHeldAxis = heldAxisForGrabbedAxis(GizmoAxis::X);
        const creative::CreativeFacadeToolDispatchReceipt r =
            appState.facade.dispatchToolInput(move);
        logMoveDispatch("MOVE", r);
      } else if (editor.frameIndex == 7U) {
        creative::CreativeToolInputPacket release;
        release.kind = creative::CreativeToolInputKind::PointerRelease;
        release.pointer.button = creative::CreativeToolPointerButton::Primary;
        release.pointer.hasWorldDestination = true;
        release.pointer.worldDestination = xAxisDestination;
        release.pointer.moveHeldAxis = heldAxisForGrabbedAxis(GizmoAxis::X);
        const creative::CreativeFacadeToolDispatchReceipt r =
            dispatchMoveReleaseWithUndo(appState, editor.undoStack, release,
                                        selectedObjectId,
                                        "capture_move_release");
        logMoveDispatch("RELEASE", r);
        if (!editor.loggedMoveAfter) {
          logObjectPlacement("AFTER",
                             appState.facade.findObject(selectedObjectId));
          editor.loggedMoveAfter = true;
        }
      }
    } else if (!editor.placeMode &&
               appState.facade.toolState().activeTool == creative::Tool::Move &&
               hasSelection) {
      // Interactive Move: while the Move tool is active and ANY object is
      // selected, hold Left-Alt (releases fly-look) and left-press. A press NEAR
      // a gizmo handle grabs that axis and maps cursor motion ALONG THAT AXIS
      // ONLY; a press away from every handle falls back to the ground-plane move
      // (camera-forward ray -> Y=0 plane). Either way the facade still owns the
      // pick + snap + commit — no per-object move math here.
      const bool* mvKeys = SDL_GetKeyboardState(nullptr);
      const bool altHeld = mvKeys != nullptr && (mvKeys[SDL_SCANCODE_LALT] != 0);
      if (altHeld) {
        float mx = 0.0F;
        float my = 0.0F;
        const SDL_MouseButtonFlags buttons = SDL_GetMouseState(&mx, &my);
        const bool lDown = (buttons & SDL_BUTTON_LMASK) != 0U;
        // High-DPI: scale logical cursor coords to drawable pixels (as select).
        const std::uint32_t logicalW = window.eventState().windowWidth;
        const std::uint32_t logicalH = window.eventState().windowHeight;
        const float scaleX =
            logicalW > 0 ? static_cast<float>(extent.width) /
                               static_cast<float>(logicalW)
                         : 1.0F;
        const float scaleY =
            logicalH > 0 ? static_cast<float>(extent.height) /
                               static_cast<float>(logicalH)
                         : 1.0F;
        const float cursorPx = mx * scaleX;
        const float cursorPy = my * scaleY;

        const creative::CreativeToolWorldPoint ground =
            resolveCreativeEditorGroundPoint(frame.camera);

        const bool selectedIsPath =
            selected != nullptr &&
            creative::describeObject(selected->kind).shapeKind ==
                creative::CreativeObjectShapeKind::Path;
        if (selectedIsPath) {
          if (lDown && !editor.interactivePathMoveActive &&
              !editor.interactivePathPointMoveActive) {
            PathPointHandleHit handle;
            if (pickPathPointHandle(pathPointHandleHits,
                                    cursorPx,
                                    cursorPy,
                                    handle)) {
              editor.interactivePathPointMoveActive = true;
              editor.interactivePathPointMoveObjectId = handle.objectId;
              editor.interactivePathPointMoveIndex = handle.pointIndex;
              editor.interactivePathPointMoveStartGround = ground;
              SDL_Log("iggy3d_creative: PATH_HANDLE interactive move begin "
                      "objectId=%llu pointIndex=%zu ground=(%.3f, %.3f, %.3f) "
                      "position=(%.3f, %.3f, %.3f) pathPoints='%s'",
                      static_cast<unsigned long long>(handle.objectId),
                      handle.pointIndex, ground.x, ground.y, ground.z,
                      handle.position.x, handle.position.y, handle.position.z,
                      pathPointsSummary(selected->pathPoints).c_str());
            } else {
              editor.interactivePathMoveActive = true;
              editor.interactivePathMoveObjectId = selectedObjectId;
              editor.interactivePathMoveStartGround = ground;
              SDL_Log("iggy3d_creative: PATH interactive move begin objectId=%llu "
                      "ground=(%.3f, %.3f, %.3f) pathPoints='%s'",
                      static_cast<unsigned long long>(selectedObjectId),
                      ground.x, ground.y, ground.z,
                      pathPointsSummary(selected->pathPoints).c_str());
            }
          } else if (!lDown && editor.interactivePathPointMoveActive) {
            const creative::CreativeVec3 delta{
                ground.x - editor.interactivePathPointMoveStartGround.x,
                0.0,
                ground.z - editor.interactivePathPointMoveStartGround.z};
            const creative::CreativeDocumentMutationReceipt receipt =
                movePathPointWithUndo(appState,
                                      editor.undoStack,
                                      editor.interactivePathPointMoveObjectId,
                                      editor.interactivePathPointMoveIndex,
                                      delta,
                                      "path_point_move_interactive_release");
            SDL_Log("iggy3d_creative: PATH_HANDLE interactive move release "
                    "objectId=%llu pointIndex=%zu status='%s' changed=%d "
                    "delta=(%.3f, %.3f, %.3f)",
                    static_cast<unsigned long long>(
                        editor.interactivePathPointMoveObjectId),
                    editor.interactivePathPointMoveIndex,
                    std::string(creative::toString(receipt.status)).c_str(),
                    receipt.changed ? 1 : 0, delta.x, delta.y, delta.z);
            editor.interactivePathPointMoveActive = false;
            editor.interactivePathPointMoveObjectId = creative::kInvalidObjectId;
            editor.interactivePathPointMoveIndex = 0U;
          } else if (!lDown && editor.interactivePathMoveActive) {
            const creative::CreativeVec3 delta{
                ground.x - editor.interactivePathMoveStartGround.x,
                0.0,
                ground.z - editor.interactivePathMoveStartGround.z};
            const creative::CreativeDocumentMutationReceipt receipt =
                movePathObjectWithUndo(appState,
                                       editor.undoStack,
                                       editor.interactivePathMoveObjectId,
                                       delta,
                                       "path_move_interactive_release");
            SDL_Log("iggy3d_creative: PATH interactive move release objectId=%llu "
                    "status='%s' changed=%d delta=(%.3f, %.3f, %.3f)",
                    static_cast<unsigned long long>(
                        editor.interactivePathMoveObjectId),
                    std::string(creative::toString(receipt.status)).c_str(),
                    receipt.changed ? 1 : 0, delta.x, delta.y, delta.z);
            editor.interactivePathMoveActive = false;
            editor.interactivePathMoveObjectId = creative::kInvalidObjectId;
          }
        } else {
        // Build the pointer packet's worldDestination + moveHeldAxis. When an
        // axis handle is grabbed, project the cursor delta since grab onto the
        // shaft's screen direction, scale it to world length along the axis, and
        // set worldDestination = S with only the grabbed axis advanced (the other
        // two pinned to S), moveHeldAxis = one of the two non-grabbed axes.
        const auto buildConstrainedDestination =
            [&](creative::CreativeToolWorldPoint& dest,
                creative::CreativeToolMoveHeldAxis& held) {
              if (editor.interactiveGrabbedAxis == GizmoAxis::None) {
                dest = ground;  // Free ground-plane move.
                held = creative::CreativeToolMoveHeldAxis::Y;
                return;
              }
              // Screen-space shaft direction at grab time (center -> tip).
              float sdx = editor.interactiveGrabTipScreen.x -
                          editor.interactiveGrabCenterScreen.x;
              float sdy = editor.interactiveGrabTipScreen.y -
                          editor.interactiveGrabCenterScreen.y;
              const float slen = std::sqrt(sdx * sdx + sdy * sdy);
              float along = 0.0F;
              if (slen > 1.0e-3F) {
                sdx /= slen;
                sdy /= slen;
                const float cdx = cursorPx - editor.interactiveGrabCursorX;
                const float cdy = cursorPy - editor.interactiveGrabCursorY;
                // pixels moved along the shaft / pixels per shaft * world length.
                const float alongPx = cdx * sdx + cdy * sdy;
                along = (alongPx / slen) * kGizmoAxisLength;
              }
              dest.x = static_cast<double>(editor.interactiveGrabAnchorS.x);
              dest.y = static_cast<double>(editor.interactiveGrabAnchorS.y);
              dest.z = static_cast<double>(editor.interactiveGrabAnchorS.z);
              switch (editor.interactiveGrabbedAxis) {
                case GizmoAxis::X:
                  dest.x += static_cast<double>(along);
                  break;
                case GizmoAxis::Y:
                  dest.y += static_cast<double>(along);
                  break;
                case GizmoAxis::Z:
                  dest.z += static_cast<double>(along);
                  break;
                case GizmoAxis::None:
                default:
                  break;
              }
              held = heldAxisForGrabbedAxis(editor.interactiveGrabbedAxis);
            };

        if (lDown && !editor.moveDragButtonDown) {
          editor.moveDragButtonDown = true;
          // Grab an axis handle if the press landed near one; else free move.
          editor.interactiveGrabbedAxis = pickGizmoAxisFromProjectedShafts(
              gizmoShafts,
              gizmoCenterScreen,
              gizmoTipScreen,
              cursorPx,
              cursorPy,
              kGizmoHandleThresholdPx);
          editor.interactiveGrabAnchorS = gizmoAnchorS;
          editor.interactiveGrabCursorX = cursorPx;
          editor.interactiveGrabCursorY = cursorPy;
          editor.interactiveGrabCenterScreen = gizmoCenterScreen;
          if (editor.interactiveGrabbedAxis == GizmoAxis::X) {
            editor.interactiveGrabTipScreen = gizmoTipScreen[0];
          } else if (editor.interactiveGrabbedAxis == GizmoAxis::Y) {
            editor.interactiveGrabTipScreen = gizmoTipScreen[1];
          } else if (editor.interactiveGrabbedAxis == GizmoAxis::Z) {
            editor.interactiveGrabTipScreen = gizmoTipScreen[2];
          }
          SDL_Log("iggy3d_creative: GIZMO grabbed axis=%s",
                  gizmoAxisName(editor.interactiveGrabbedAxis));
          creative::CreativeToolInputPacket press;
          press.kind = creative::CreativeToolInputKind::PointerPress;
          press.pointer.button = creative::CreativeToolPointerButton::Primary;
          press.pointer.target =
              creative::TargetRef{static_cast<creative::Id>(selectedObjectId)};
          (void)appState.facade.dispatchToolInput(press);
        } else if (lDown && editor.moveDragButtonDown) {
          creative::CreativeToolInputPacket move;
          move.kind = creative::CreativeToolInputKind::PointerMove;
          move.pointer.button = creative::CreativeToolPointerButton::Primary;
          move.pointer.hasWorldDestination = true;
          buildConstrainedDestination(move.pointer.worldDestination,
                                      move.pointer.moveHeldAxis);
          (void)appState.facade.dispatchToolInput(move);
        } else if (!lDown && editor.moveDragButtonDown) {
          editor.moveDragButtonDown = false;
          creative::CreativeToolInputPacket release;
          release.kind = creative::CreativeToolInputKind::PointerRelease;
          release.pointer.button = creative::CreativeToolPointerButton::Primary;
          release.pointer.hasWorldDestination = true;
          buildConstrainedDestination(release.pointer.worldDestination,
                                      release.pointer.moveHeldAxis);
          const creative::CreativeFacadeToolDispatchReceipt r =
              dispatchMoveReleaseWithUndo(appState, editor.undoStack, release,
                                          selectedObjectId,
                                          "move_interactive_release");
          logMoveDispatch("RELEASE", r);
          editor.interactiveGrabbedAxis = GizmoAxis::None;
        }
        }
      } else if (editor.moveDragButtonDown) {
        editor.moveDragButtonDown = false;  // Alt released mid-drag: drop the latch.
        editor.interactiveGrabbedAxis = GizmoAxis::None;
      } else if (editor.interactivePathMoveActive) {
        SDL_Log("iggy3d_creative: PATH interactive move cancelled objectId=%llu",
                static_cast<unsigned long long>(
                    editor.interactivePathMoveObjectId));
        editor.interactivePathMoveActive = false;
        editor.interactivePathMoveObjectId = creative::kInvalidObjectId;
      } else if (editor.interactivePathPointMoveActive) {
        SDL_Log("iggy3d_creative: PATH_HANDLE interactive move cancelled "
                "objectId=%llu pointIndex=%zu",
                static_cast<unsigned long long>(
                    editor.interactivePathPointMoveObjectId),
                editor.interactivePathPointMoveIndex);
        editor.interactivePathPointMoveActive = false;
        editor.interactivePathPointMoveObjectId = creative::kInvalidObjectId;
        editor.interactivePathPointMoveIndex = 0U;
      }
    }

    // ---- INSPECTOR UI (draw list -> menu frame rects + glyphs) -------------
    ProductCreativeUiProjectionRequest uiReq;
    uiReq.creative = &appState;  // model=nullptr -> facade.buildUiModel().
    uiReq.virtualWidth = 1280;
    uiReq.virtualHeight = 720;
    const ProductCreativeUiProjection uiProj =
        buildProductCreativeUiProjection(uiReq);

    ProductVulkanMenuFrameRequest menuReq;
    menuReq.uiDrawList = &uiProj.drawList;
    menuReq.frameIndex = editor.frameIndex;
    menuReq.drawableWidth = extent.width;
    menuReq.drawableHeight = extent.height;
    ProductVulkanMenuFrame menuFrame =
        buildProductVulkanStarterMenuFrame(menuReq);

    // ---- BOUNDS BOX (wireframe) --------------------------------------------
    const creative::CreativeDocumentWireframeSegmentBuildResult segs =
        creative::buildCreativeDocumentWireframeSegments(
            appState.facade.document(), wireProjReq);
    ProductCreativeWireframeDebugLineBuildResult lines =
        buildProductCreativeWireframeDebugLines(segs.segmentList);
    // The renderer turns each line into a world-space tube of `thickness` METRES
    // (creativeDebugLineBox: size = |edge| x thickness x thickness), so keep it
    // thin (a few cm) or a 1 m box fills into a solid blob. Selected edges go
    // bright yellow + a touch fatter for emphasis (the kernel colors by style,
    // not by selection).
    for (ProductCreativeWireframeDebugLine& line : lines.lineList.lines) {
      // Recolor the SELECTED object's edges — matched by id, whatever the kind.
      const bool sel = hasSelection &&
                       line.objectId == static_cast<creative::CreativeObjectId>(
                                            selectedId);
      line.thickness = sel ? 0.06F : 0.03F;
      if (sel) {
        line.color = {1.0F, 1.0F, 0.0F, 1.0F};
      }
    }
    ProductCreativeWireframeDebugRenderFrame dbg =
        buildProductCreativeWireframeDebugRenderFrame(&lines.lineList);

    // ---- GIZMO WIREFRAME ----------------------------------------------------
    // Build ONE combined line vector: the document wireframe lines that draw the
    // yellow selection box (dbg.lines, already converted to render lines) PLUS
    // the 3 axis-aligned gizmo shafts. Point frame.creativeWireframeDebug at THIS
    // vector so the renderer draws both. The vector must outlive submitFrame(),
    // so it lives here in the frame-loop body. When nothing is selected we skip
    // the gizmo and the selection box is empty, so this is just dbg.lines.
    std::vector<RenderCreativeWireframeDebugLine> combinedWireLines;
    combinedWireLines.reserve(dbg.lines.size() + 48);
    std::size_t documentWireLineCount = 0;
    for (const RenderCreativeWireframeDebugLine& line : dbg.lines) {
      const creative::CreativeObject* object =
          line.objectId != creative::kInvalidObjectId
              ? appState.facade.findObject(line.objectId)
              : nullptr;
      if (object != nullptr &&
          creative::describeObject(object->kind).shapeKind ==
              creative::CreativeObjectShapeKind::Line) {
        continue;
      }
      combinedWireLines.push_back(line);
    }
    documentWireLineCount = combinedWireLines.size();
    std::size_t pointMarkerEdgeCount = 0;
    std::size_t lineMarkerEdgeCount = 0;
    std::size_t pathPointHandleEdgeCount = 0;
    for (const creative::CreativeObject& obj :
         appState.facade.document().objects()) {
      const creative::CreativeObjectDescriptor& descriptor =
          creative::describeObject(obj.kind);
      if (!obj.visible) {
        continue;
      }
      const bool sel =
          hasSelection &&
          obj.id == static_cast<creative::CreativeObjectId>(selectedId);
      if (descriptor.shapeKind != creative::CreativeObjectShapeKind::Point &&
          descriptor.shapeKind != creative::CreativeObjectShapeKind::Line) {
        continue;
      }
      const VisualBounds markerBounds = visualBoundsForObject(obj);
      const std::size_t before = combinedWireLines.size();
      appendStandaloneWireframeBoxEdges(
          combinedWireLines, markerBounds.min, markerBounds.max,
          sel ? RenderLineColor{1.0F, 1.0F, 0.0F, 1.0F}
              : descriptor.shapeKind == creative::CreativeObjectShapeKind::Line
                    ? RenderLineColor{0.86F, 0.68F, 0.28F, 1.0F}
                    : RenderLineColor{0.34F, 0.62F, 0.88F, 1.0F},
          sel ? 0.06F : 0.035F);
      for (std::size_t i = before; i < combinedWireLines.size(); ++i) {
        combinedWireLines[i].objectId = obj.id;
      }
      if (descriptor.shapeKind == creative::CreativeObjectShapeKind::Line) {
        lineMarkerEdgeCount += combinedWireLines.size() - before;
      } else {
        pointMarkerEdgeCount += combinedWireLines.size() - before;
      }
    }
    if (selectedIsPathForHandles) {
      for (const creative::CreativePathPoint& point : selected->pathPoints) {
        const VisualBounds handleBounds = pathPointHandleBounds(point.position);
        const std::size_t before = combinedWireLines.size();
        appendStandaloneWireframeBoxEdges(
            combinedWireLines,
            handleBounds.min,
            handleBounds.max,
            RenderLineColor{0.20F, 0.88F, 1.0F, 1.0F},
            0.035F);
        for (std::size_t i = before; i < combinedWireLines.size(); ++i) {
          combinedWireLines[i].objectId =
              static_cast<creative::CreativeObjectId>(selectedId);
        }
        pathPointHandleEdgeCount += combinedWireLines.size() - before;
      }
    }
    if (hasSelection) {
      for (const GizmoAxisShaft& shaft : gizmoShafts) {
        RenderCreativeWireframeDebugLine gizmoLine;
        gizmoLine.start = gizmoCenter;
        gizmoLine.end = shaft.tip;  // Axis-aligned: only one component differs.
        gizmoLine.color = shaft.color;
        gizmoLine.objectId =
            static_cast<creative::CreativeObjectId>(selectedId);
        gizmoLine.thickness = kGizmoThickness;
        combinedWireLines.push_back(gizmoLine);
      }
    }
    // ---- GHOST PREVIEW ------------------------------------------------------
    // In Place mode, draw a GREEN (0,1,0,1) axis-aligned wireframe box at the
    // aimed cell sized to the current brush footprint (min.y=0..height, XZ
    // centered on the cell) — the placement preview. It rides the SAME combined
    // wireframe vector as the selection box + gizmo, so it needs no new render
    // path. It is NOT a document object (objectId=0); it vanishes on the drop's
    // next frame if the aim moves.
    std::size_t ghostEdgeCount = 0;
    if (editor.placeMode) {
      const creative::CreativeObjectDescriptor& brushDescriptor =
          creative::describeObject(editor.placeBrush);
      Vec3 ghostMin{};
      Vec3 ghostMax{};
      if (brushDescriptor.shapeKind == creative::CreativeObjectShapeKind::Path) {
        const std::vector<creative::CreativePathPoint> ghostPath =
            initialPathPointsForAnchor(aimCellCenter);
        const std::size_t before = combinedWireLines.size();
        appendPathPolylineLines(combinedWireLines,
                                ghostPath,
                                RenderLineColor{0.0F, 1.0F, 0.0F, 1.0F},
                                kGizmoThickness);
        ghostEdgeCount = combinedWireLines.size() - before;
      } else if (brushDescriptor.shapeKind ==
                 creative::CreativeObjectShapeKind::Point) {
        const VisualBounds markerBounds = pointMarkerBounds(
            creative::CreativeVec3{aimCellCenter.x, 0.0, aimCellCenter.z});
        ghostMin = markerBounds.min;
        ghostMax = markerBounds.max;
      } else {
        const BrushFootprint fp = brushFootprintForDescriptor(brushDescriptor);
        const VisualBounds authoredGhost{
            {aimCellCenter.x - fp.sizeX * 0.5F, 0.0F,
             aimCellCenter.z - fp.sizeZ * 0.5F},
            {aimCellCenter.x + fp.sizeX * 0.5F, fp.height,
             aimCellCenter.z + fp.sizeZ * 0.5F}};
        const VisualBounds ghostBounds =
            brushDescriptor.shapeKind == creative::CreativeObjectShapeKind::Line
                ? lineProxyBounds(authoredGhost)
                : authoredGhost;
        ghostMin = ghostBounds.min;
        ghostMax = ghostBounds.max;
      }
      if (brushDescriptor.shapeKind != creative::CreativeObjectShapeKind::Path) {
        const std::size_t before = combinedWireLines.size();
        appendStandaloneWireframeBoxEdges(
            combinedWireLines, ghostMin, ghostMax,
            RenderLineColor{0.0F, 1.0F, 0.0F, 1.0F},
            kGizmoThickness);
        ghostEdgeCount = combinedWireLines.size() - before;
      }
    }
    (void)ghostEdgeCount;
    RenderCreativeWireframeDebugFrame combinedWireFrame;
    combinedWireFrame.available = true;
    combinedWireFrame.visible = !combinedWireLines.empty();
    combinedWireFrame.lines = combinedWireLines.data();
    combinedWireFrame.lineCount = combinedWireLines.size();

    // ---- DIMENSION LABEL + glyph merge -------------------------------------
    // Merge the inspector-panel glyphs with the dimension-label glyphs into ONE
    // vector so a single .data() pointer stays valid for the whole frame.
    std::vector<DebugHudGlyphQuad> glyphs = menuFrame.textGlyphQuads;
    if (hasSelection) {
      const Vec3 center{(selBoxMin.x + selBoxMax.x) * 0.5F,
                        (selBoxMin.y + selBoxMax.y) * 0.5F,
                        (selBoxMin.z + selBoxMax.z) * 0.5F};
      const ProjectedPoint3 projected =
          projectPoint(frame.camera.clipFromWorld, center);
      if (std::isfinite(projected.w) && projected.w > 0.0F) {
        const Vec3 ndc = projected.ndc;
        const float px = (ndc.x * 0.5F + 0.5F) * static_cast<float>(extent.width);
        const float py = (1.0F - (ndc.y * 0.5F + 0.5F)) *
                         static_cast<float>(extent.height);
        const float dimW = selBoxMax.x - selBoxMin.x;
        const float dimH = selBoxMax.y - selBoxMin.y;
        const float dimD = selBoxMax.z - selBoxMin.z;
        char labelBuf[64];
        std::snprintf(labelBuf, sizeof(labelBuf), "%.1f x %.1f x %.1f m",
                      static_cast<double>(dimW), static_cast<double>(dimH),
                      static_cast<double>(dimD));
        const DebugHudLayoutResult labelLayout = layoutDebugHudTextAt(
            labelBuf, static_cast<std::int32_t>(px),
            static_cast<std::int32_t>(py), extent.width, extent.height);
        glyphs.insert(glyphs.end(), labelLayout.quads.begin(),
                      labelLayout.quads.end());
      }
    }

    // ---- ATTACH overlays to the frame --------------------------------------
    frame.ui.visible = true;
    frame.ui.rects = menuFrame.rects.data();
    frame.ui.rectCount = menuFrame.rects.size();
    frame.ui.textGlyphQuads = glyphs.data();
    frame.ui.textGlyphQuadCount = glyphs.size();
    // The combined vector (selection box + gizmo shafts), NOT dbg.frame.
    frame.creativeWireframeDebug = combinedWireFrame;

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
              documentWireLineCount, pointMarkerEdgeCount, lineMarkerEdgeCount,
              pathPointHandleEdgeCount,
              combinedWireLines.size() - documentWireLineCount -
                  pointMarkerEdgeCount - lineMarkerEdgeCount -
                  pathPointHandleEdgeCount,
              combinedWireLines.size(), menuFrame.rects.size(), glyphs.size());
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
              hasSelection ? 1 : 0, documentWireLineCount,
              pointMarkerEdgeCount, lineMarkerEdgeCount,
              pathPointHandleEdgeCount,
              combinedWireLines.size() - documentWireLineCount -
                  pointMarkerEdgeCount - lineMarkerEdgeCount -
                  pathPointHandleEdgeCount,
              combinedWireLines.size(), editor.placeMode ? 1 : 0,
              std::string(creative::toString(editor.placeBrush)).c_str(),
              ghostEdgeCount,
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
