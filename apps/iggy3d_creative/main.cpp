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

#include "CreativeRendererBootstrap.hpp"
#include "StandaloneCaptureScenario.hpp"
#include "StandaloneCaptureScript.hpp"
#include "StandaloneBrushPalette.hpp"
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
using iggy3d_creative_app::buildObjectVisualPickBounds;
using iggy3d_creative_app::brushFootprintForDescriptor;
using iggy3d_creative_app::buildBrushPaletteFromDescriptors;
using iggy3d_creative_app::buildStandaloneRoomBakePreviewScene;
using iggy3d_creative_app::captureFrameToPng;
using iggy3d_creative_app::clearToBlankScene;
using iggy3d_creative_app::clearUndoStack;
using iggy3d_creative_app::createCreativeRenderer;
using iggy3d_creative_app::firstBrushKind;
using iggy3d_creative_app::dispatchMoveReleaseWithUndo;
using iggy3d_creative_app::GizmoAxis;
using iggy3d_creative_app::GizmoAxisShaft;
using iggy3d_creative_app::gizmoAxisName;
using iggy3d_creative_app::heldAxisForGrabbedAxis;
using iggy3d_creative_app::initialPathPointsForAnchor;
using iggy3d_creative_app::loadStandaloneScene;
using iggy3d_creative_app::logMoveDispatch;
using iggy3d_creative_app::logObjectPlacement;
using iggy3d_creative_app::movePathObjectWithUndo;
using iggy3d_creative_app::movePathPointWithUndo;
using iggy3d_creative_app::pushUndoSnapshot;
using iggy3d_creative_app::nextBrushKind;
using iggy3d_creative_app::appendPathPolylineLines;
using iggy3d_creative_app::buildPathPointHandleHits;
using iggy3d_creative_app::lineProxyBounds;
using iggy3d_creative_app::ObjectVisualPickBounds;
using iggy3d_creative_app::ObjectVisualPickResult;
using iggy3d_creative_app::pathPointHandleBounds;
using iggy3d_creative_app::PathPointHandleHit;
using iggy3d_creative_app::pickNearestVisualBoundsObject;
using iggy3d_creative_app::pickPathPointHandle;
using iggy3d_creative_app::pickGizmoAxisFromProjectedShafts;
using iggy3d_creative_app::pointMarkerBounds;
using iggy3d_creative_app::placeBrushObjectWithUndo;
using iggy3d_creative_app::pathPointsSummary;
using iggy3d_creative_app::projectBoxToScreen;
using iggy3d_creative_app::projectPointToScreen;
using iggy3d_creative_app::ScreenPoint;
using iggy3d_creative_app::snapGroundToCellCenter;
using iggy3d_creative_app::saveStandaloneScene;
using iggy3d_creative_app::StandaloneRoomBakePreviewScene;
using iggy3d_creative_app::appendStandaloneWireframeBoxEdges;
using iggy3d_creative_app::toVec3;
using iggy3d_creative_app::undoLastSnapshot;
using iggy3d_creative_app::validPathPoints;
using iggy3d_creative_app::VisualBounds;
using iggy3d_creative_app::visualBoundsCenter;
using iggy3d_creative_app::visualBoundsForObject;
using iggy3d_creative_app::WorldRay;
using iggy3d_creative_app::worldRayFromPixel;
using iggy3d_creative_app::logStandaloneRoomBakeFinal;

// Delete the currently selected object through the facade's generic document
// removal seam. The facade owns invalidating selection/tool/ghost/measurement
// references; this app only asks to remove the selected target id and logs proof.
creative::CreativeDocumentRemoveReceipt deleteSelectedObject(
    creative::CreativeAppState& appState, std::string_view source,
    StandaloneUndoStack* undoStack = nullptr) {
  const creative::Id selectedId =
      appState.facade.selectionState().selectedTarget.value;
  const std::uint64_t objectCountBefore =
      static_cast<std::uint64_t>(appState.facade.document().objectCount());
  if (selectedId == 0U) {
    SDL_Log("iggy3d_creative: DELETE no selection source='%s' "
            "objectCount=%llu",
            std::string(source).c_str(),
            static_cast<unsigned long long>(objectCountBefore));
    return {};
  }

  const auto objectId = static_cast<creative::CreativeObjectId>(selectedId);
  const creative::CreativeObject* object = appState.facade.findObject(objectId);
  if (object == nullptr) {
    SDL_Log("iggy3d_creative: DELETE missing selection source='%s' "
            "objectId=%llu objectCount=%llu",
            std::string(source).c_str(),
            static_cast<unsigned long long>(objectId),
            static_cast<unsigned long long>(objectCountBefore));
    return {};
  }

  const creative::CreativeObjectKind kind = object->kind;
  const std::size_t undoDepthBefore =
      undoStack != nullptr ? undoStack->documents.size() : 0U;
  if (undoStack != nullptr) {
    pushUndoSnapshot(*undoStack, appState.facade, source);
  }
  creative::CreativeDocumentRemoveReceipt receipt =
      appState.facade.removeDocumentObject(objectId);
  if ((!receipt.accepted || !receipt.objectRemoved) && undoStack != nullptr &&
      undoStack->documents.size() > undoDepthBefore) {
    undoStack->documents.pop_back();
    SDL_Log("iggy3d_creative: UNDO discarded source='%s' depth=%zu "
            "reasonCode='%s'",
            std::string(source).c_str(), undoStack->documents.size(),
            std::string(receipt.reasonCode).c_str());
  }
  const std::uint64_t objectCountAfter =
      static_cast<std::uint64_t>(appState.facade.document().objectCount());
  const creative::Id selectionAfter =
      appState.facade.selectionState().selectedTarget.value;
  SDL_Log("iggy3d_creative: DELETE removed objectId=%llu kind='%s' "
          "accepted=%d changed=%d removed=%d status='%s' reasonCode='%s' "
          "objectCountBefore=%llu objectCountAfter=%llu selectionAfter=%u",
          static_cast<unsigned long long>(objectId),
          std::string(creative::toString(kind)).c_str(),
          receipt.accepted ? 1 : 0, receipt.changed ? 1 : 0,
          receipt.objectRemoved ? 1 : 0,
          std::string(creative::toString(receipt.status)).c_str(),
          std::string(receipt.reasonCode).c_str(),
          static_cast<unsigned long long>(objectCountBefore),
          static_cast<unsigned long long>(objectCountAfter), selectionAfter);
  return receipt;
}

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

  // Fly camera state. Start pulled back and up, looking at the origin.
  ProductCreativeFlyConfig flyConfig;
  flyConfig.enabled = true;
  flyConfig.speedMetersPerSecond = 8.0F;
  flyConfig.sprintMultiplier = 3.0F;
  flyConfig.inputStepSeconds = 1.0F / 60.0F;

  Vec3 flyPos{0.0F, 6.0F, 12.0F};
  float yawDegrees = 0.0F;
  float pitchDegrees = -25.0F;

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

  // Selected-state logging: emit the selection + submit reason once.
  bool loggedSelection = false;

  // ---- MOVE state --------------------------------------------------------
  // Interactive: edge-triggered key latches for '1' Select / '2' Move so a held
  // key switches the tool exactly once. Interactive drag latch tracks a left
  // button held while the Move tool is active.
  bool prevKey1 = false;
  bool prevKey2 = false;
  bool moveDragButtonDown = false;
  // --capture: run the generic Move lifecycle across a few frames, and log the
  // crate placement BEFORE the commit and AFTER the release exactly once.
  bool loggedMoveBefore = false;
  bool loggedMoveAfter = false;
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
  GizmoAxis interactiveGrabbedAxis = GizmoAxis::None;
  Vec3 interactiveGrabAnchorS{0.0F, 0.0F, 0.0F};
  float interactiveGrabCursorX = 0.0F;
  float interactiveGrabCursorY = 0.0F;
  ScreenPoint interactiveGrabCenterScreen;
  ScreenPoint interactiveGrabTipScreen;
  bool interactivePathMoveActive = false;
  creative::CreativeObjectId interactivePathMoveObjectId =
      creative::kInvalidObjectId;
  creative::CreativeToolWorldPoint interactivePathMoveStartGround{};
  bool interactivePathPointMoveActive = false;
  creative::CreativeObjectId interactivePathPointMoveObjectId =
      creative::kInvalidObjectId;
  std::size_t interactivePathPointMoveIndex = 0U;
  creative::CreativeToolWorldPoint interactivePathPointMoveStartGround{};
  // --capture: log the grabbed axis exactly once.
  bool loggedGizmoGrab = false;

  // ---- PLACE state -------------------------------------------------------
  // placeMode is an APP-level mode (not a kernel Tool) toggled by '3'. When on,
  // the click drops a NEW object at the aimed cell instead of running the
  // select/move hit-test. '1'/'2' leave place mode and set the kernel tool.
  // placeBrush is the current descriptor-backed brush kind. The grid pitch
  // (1 m) is the placement cell size for snapping.
  bool placeMode = false;
  const std::vector<creative::CreativeObjectKind> brushPalette =
      buildBrushPaletteFromDescriptors();
  creative::CreativeObjectKind placeBrush = firstBrushKind(brushPalette);
  SDL_Log("iggy3d_creative: brush palette slots=%llu first='%s'",
          static_cast<unsigned long long>(brushPalette.size()),
          std::string(creative::toString(placeBrush)).c_str());
  const double placeCellSize = static_cast<double>(gridConfig.pitchMeters);
  bool prevKey3 = false;
  bool prevKeyB = false;
  bool placeButtonDown = false;  // Interactive left-button edge latch in Place.
  std::uint64_t placedCount = 0;  // Objects dropped via Place (for the count log).
  // --capture: in Place mode we start ON so the proof frames can drop objects.
  if (!capturePath.empty()) {
    placeMode = true;
    placeBrush = firstBrushKind(brushPalette);
  }
  // ---- SAVE / LOAD state ------------------------------------------------
  // Interactive: edge latches for F5 (save), F6 (new/clear), F9 (load).
  bool prevKeyF5 = false;
  bool prevKeyF6 = false;
  bool prevKeyF9 = false;
  bool prevKeyDelete = false;
  bool prevKeyBackspace = false;
  bool prevKeyZ = false;
  StandaloneUndoStack undoStack;
  // --capture round-trip proof: prove create/delete/move undo, add Point and
  // Line + Path markers, undo their moves, then SAVE/CLEAR/LOAD the
  // eight-object scene. The schedule, flags, ids, and snapshots live in the
  // capture script helper; main only executes the current frame's authored step.
  StandaloneCaptureScript captureScript;
  bool captureWorldPickFloorLogged = false;
  bool captureWorldPickPointLogged = false;
  bool captureWorldPickLineLogged = false;
  bool captureWorldPickPathLogged = false;

  std::uint64_t frameIndex = 0;
  std::uint32_t lastWidth = 0;
  std::uint32_t lastHeight = 0;

  constexpr float kMouseSensitivity = 0.12F;

  while (window.isOpen()) {
    window.pollEvents();
    if (window.eventState().quitRequested) {
      break;
    }
    if (!window.isDrawable()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
      continue;
    }

    const SdlDrawableExtent extent = window.drawableExtent();
    if (extent.width == 0U || extent.height == 0U) {
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
      continue;
    }
    if (extent.width != lastWidth || extent.height != lastHeight) {
      RenderViewport viewport;
      viewport.width = extent.width;
      viewport.height = extent.height;
      viewport.aspectRatio =
          static_cast<float>(extent.width) / static_cast<float>(extent.height);
      backend->resize(viewport);
      lastWidth = extent.width;
      lastHeight = extent.height;
    }

    // INPUT: keyboard (WASD move, Space/LCtrl up/down, LShift sprint) +
    // relative mouse (look).
    const bool* keys = SDL_GetKeyboardState(nullptr);
    ProductCreativeFlyInput flyInput;
    if (keys != nullptr) {
      // Fly camera axes: direction = right*moveX + forward*moveY + up*moveZ.
      // So moveX = strafe (A/D), moveY = forward/back (W/S), moveZ = up/down
      // (Space/LCtrl).
      const float moveX = (keys[SDL_SCANCODE_D] ? 1.0F : 0.0F) -
                          (keys[SDL_SCANCODE_A] ? 1.0F : 0.0F);
      const float moveY = (keys[SDL_SCANCODE_W] ? 1.0F : 0.0F) -
                          (keys[SDL_SCANCODE_S] ? 1.0F : 0.0F);
      const float moveZ = (keys[SDL_SCANCODE_SPACE] ? 1.0F : 0.0F) -
                          (keys[SDL_SCANCODE_LCTRL] ? 1.0F : 0.0F);
      flyInput.moveX = moveX;
      flyInput.moveY = moveY;
      flyInput.moveZ = moveZ;
      flyInput.sprinting = keys[SDL_SCANCODE_LSHIFT];
    }

    float mouseDx = 0.0F;
    float mouseDy = 0.0F;
    SDL_GetRelativeMouseState(&mouseDx, &mouseDy);
    yawDegrees += mouseDx * kMouseSensitivity;
    pitchDegrees =
        std::clamp(pitchDegrees - mouseDy * kMouseSensitivity, -80.0F, 80.0F);
    flyInput.cameraYawDegrees = yawDegrees;
    flyInput.cameraPitchDegrees = pitchDegrees;

    // CAMERA: advance the fly position.
    const ProductCreativeFlyResult flyResult =
        applyProductCreativeFlyInput(flyConfig, flyInput, flyPos);
    if (flyResult.applied) {
      flyPos = flyResult.finalPositionMeters;
    }

    // ---- TOOL SWITCH: '1' -> Select, '2' -> Move ------------------
    // Edge-triggered so a held key flips the active tool once. Driven ONLY
    // through the facade's generic setActiveTool — no per-tool special-casing.
    if (capturePath.empty() && keys != nullptr) {
      const bool key1 = keys[SDL_SCANCODE_1] != 0;
      const bool key2 = keys[SDL_SCANCODE_2] != 0;
      const bool key3 = keys[SDL_SCANCODE_3] != 0;
      const bool keyB = keys[SDL_SCANCODE_B] != 0;
      const bool keyDelete = keys[SDL_SCANCODE_DELETE] != 0;
      const bool keyBackspace = keys[SDL_SCANCODE_BACKSPACE] != 0;
      const bool keyZ = keys[SDL_SCANCODE_Z] != 0;
      const SDL_Keymod modState = SDL_GetModState();
      const bool undoModifier =
          (modState & (SDL_KMOD_GUI | SDL_KMOD_CTRL)) != 0U;
      if (key1 && !prevKey1) {
        placeMode = false;  // '1' Select leaves Place mode.
        const bool ok = appState.facade.setActiveTool(creative::Tool::Select);
        SDL_Log("iggy3d_creative: setActiveTool(Select) accepted=%d placeMode=0",
                ok ? 1 : 0);
      }
      if (key2 && !prevKey2) {
        placeMode = false;  // '2' Move leaves Place mode.
        const bool ok = appState.facade.setActiveTool(creative::Tool::Move);
        SDL_Log("iggy3d_creative: setActiveTool(Move) accepted=%d placeMode=0",
                ok ? 1 : 0);
      }
      if (key3 && !prevKey3) {
        placeMode = true;  // '3' Place: app-level mode, not a kernel Tool.
        SDL_Log("iggy3d_creative: placeMode=1 brush='%s'",
                std::string(creative::toString(placeBrush)).c_str());
      }
      if (keyB && !prevKeyB) {
        placeBrush = nextBrushKind(brushPalette, placeBrush);
        SDL_Log("iggy3d_creative: brush cycled -> '%s'",
                std::string(creative::toString(placeBrush)).c_str());
      }
      if ((keyDelete && !prevKeyDelete) ||
          (keyBackspace && !prevKeyBackspace)) {
        (void)deleteSelectedObject(
            appState, keyDelete ? "delete_key" : "backspace_key", &undoStack);
      }
      if (keyZ && !prevKeyZ && undoModifier) {
        (void)undoLastSnapshot(appState, undoStack, "keyboard_undo");
      }
      // ---- SAVE / LOAD keys: F5 save, F6 new/clear, F9 load -------
      const bool keyF5 = keys[SDL_SCANCODE_F5] != 0;
      const bool keyF6 = keys[SDL_SCANCODE_F6] != 0;
      const bool keyF9 = keys[SDL_SCANCODE_F9] != 0;
      if (keyF5 && !prevKeyF5) {
        const CreativeWorldSaveResult saveResult =
            saveStandaloneScene(appState.facade, saveRoot, saveId);
        if (saveResult.accepted && saveResult.saved) {
          clearUndoStack(undoStack, "save_success");
        }
      }
      if (keyF6 && !prevKeyF6) {
        clearToBlankScene(appState);  // Fresh blank document; facade resets
                                      // selection so no stale seed id dangles.
        clearUndoStack(undoStack, "new_clear");
      }
      if (keyF9 && !prevKeyF9) {
        const bool loaded = loadStandaloneScene(appState, saveRoot, saveId);
        if (loaded) {
          clearUndoStack(undoStack, "load_success");
        }
      }
      prevKey1 = key1;
      prevKey2 = key2;
      prevKey3 = key3;
      prevKeyB = keyB;
      prevKeyDelete = keyDelete;
      prevKeyBackspace = keyBackspace;
      prevKeyZ = keyZ;
      prevKeyF5 = keyF5;
      prevKeyF6 = keyF6;
      prevKeyF9 = keyF9;
    }

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
        scene, debug, frameIndex++, extent.width, extent.height, yawDegrees,
        pitchDegrees, /*cameraAnchorOverrideAvailable=*/true, flyPos);

    // ---- AIM -> GROUND CELL (Place mode) --------------------------------
    // Cast the camera-forward ray to the Y=0 plane (eye + forward*t), giving a
    // world XZ ground point, then snap XZ to the nearest 1 m cell center. This
    // is the SAME camera-ray -> Y=0 math the interactive ground-plane Move uses;
    // it feeds both the green ghost preview and the drop position. When the ray
    // is (near) parallel to the ground we fall back to the point under the eye.
    const Vec3 aimEye = frame.camera.worldEye;
    const Vec3 aimFwd = frame.camera.worldForward;
    double aimGroundX = static_cast<double>(aimEye.x);
    double aimGroundZ = static_cast<double>(aimEye.z);
    if (std::fabs(aimFwd.y) > 1.0e-4F) {
      const float t = -aimEye.y / aimFwd.y;  // eye.y + t*fwd.y == 0
      if (t > 0.0F) {
        aimGroundX = static_cast<double>(aimEye.x + aimFwd.x * t);
        aimGroundZ = static_cast<double>(aimEye.z + aimFwd.z * t);
      }
    }
    const Vec3 aimCellCenter =
        snapGroundToCellCenter(aimGroundX, aimGroundZ, placeCellSize);

    // ---- CLICK-TO-SELECT (generic over ALL objects) ------------------------
    // Scan every visible object's visual bounds with one world-space ray. The
    // nearest ray-entry distance wins, so overlapping projected boxes select the
    // closest surface instead of the object with the nearest projected center.
    std::vector<ObjectVisualPickBounds> objectPickCandidates;
    // Remember the FLOOR's world bounds so --capture can aim its click at a point
    // on the floor OUTSIDE the crate's footprint (the crate sits over the floor's
    // center, so a center-click would land on the nearer crate — nearest wins).
    bool haveFloorBounds = false;
    Vec3 floorBoxMin{};
    Vec3 floorBoxMax{};
    for (const creative::CreativeObject& obj : appState.facade.document().objects()) {
      if (!obj.visible) {
        continue;
      }
      const ObjectVisualPickBounds hit = buildObjectVisualPickBounds(
          obj, frame.camera.clipFromWorld, extent.width, extent.height);
      const Vec3 boxMin = hit.bounds.min;
      const Vec3 boxMax = hit.bounds.max;
      objectPickCandidates.push_back(hit);
      if (obj.id == floorObjectId) {
        haveFloorBounds = true;
        floorBoxMin = boxMin;
        floorBoxMax = boxMax;
      }
      if (!capturePath.empty() && !captureScript.pointHitProxyLogged &&
          obj.id == captureScript.pointTargetId) {
        SDL_Log("iggy3d_creative: POINT hit proxy objectId=%llu "
                "aabbValid=%d marker=[(%.3f, %.3f, %.3f).."
                "(%.3f, %.3f, %.3f)] screen=[%.1f, %.1f..%.1f, %.1f]",
                static_cast<unsigned long long>(captureScript.pointTargetId),
                hit.screenAabb.valid ? 1 : 0, boxMin.x, boxMin.y, boxMin.z,
                boxMax.x, boxMax.y, boxMax.z, hit.screenAabb.minX,
                hit.screenAabb.minY, hit.screenAabb.maxX,
                hit.screenAabb.maxY);
        captureScript.pointHitProxyLogged = true;
      }
      if (!capturePath.empty() && !captureScript.lineHitProxyLogged &&
          obj.id == captureScript.lineTargetId) {
        SDL_Log("iggy3d_creative: LINE hit proxy objectId=%llu "
                "aabbValid=%d visual=[(%.3f, %.3f, %.3f).."
                "(%.3f, %.3f, %.3f)] screen=[%.1f, %.1f..%.1f, %.1f]",
                static_cast<unsigned long long>(captureScript.lineTargetId),
                hit.screenAabb.valid ? 1 : 0, boxMin.x, boxMin.y, boxMin.z,
                boxMax.x, boxMax.y, boxMax.z, hit.screenAabb.minX,
                hit.screenAabb.minY, hit.screenAabb.maxX,
                hit.screenAabb.maxY);
        captureScript.lineHitProxyLogged = true;
      }
      if (!capturePath.empty() && !captureScript.pathHitProxyLogged &&
          obj.id == captureScript.pathTargetId) {
        SDL_Log("iggy3d_creative: PATH hit proxy objectId=%llu "
                "aabbValid=%d visual=[(%.3f, %.3f, %.3f).."
                "(%.3f, %.3f, %.3f)] screen=[%.1f, %.1f..%.1f, %.1f] "
                "pathPointCount=%zu pathPoints='%s'",
                static_cast<unsigned long long>(captureScript.pathTargetId),
                hit.screenAabb.valid ? 1 : 0, boxMin.x, boxMin.y, boxMin.z,
                boxMax.x, boxMax.y, boxMax.z, hit.screenAabb.minX,
                hit.screenAabb.minY, hit.screenAabb.maxX,
                hit.screenAabb.maxY, obj.pathPoints.size(),
                pathPointsSummary(obj.pathPoints).c_str());
        captureScript.pathHitProxyLogged = true;
      }
    }

    if (!capturePath.empty()) {
      auto logWorldPickProof =
          [&](const char* label, creative::CreativeObjectId expectedId,
              Vec3 worldPoint, bool& logged) {
            if (logged || expectedId == creative::kInvalidObjectId) {
              return;
            }
            const ScreenPoint screenPoint = projectPointToScreen(
                frame.camera.clipFromWorld, worldPoint, extent.width,
                extent.height);
            if (!screenPoint.valid) {
              return;
            }
            const WorldRay ray =
                worldRayFromPixel(frame.camera, screenPoint.x, screenPoint.y,
                                  extent.width, extent.height);
            const ObjectVisualPickResult pick =
                pickNearestVisualBoundsObject(objectPickCandidates, ray);
            SDL_Log("iggy3d_creative: WORLD_PICK_PROOF label='%s' "
                    "click=(%.1f, %.1f) rayValid=%d expectedObjectId=%llu "
                    "pickedObjectId=%llu matched=%d entryDistance=%.3f "
                    "tested=%llu hits=%llu",
                    label, screenPoint.x, screenPoint.y,
                    pick.rayValid ? 1 : 0,
                    static_cast<unsigned long long>(expectedId),
                    static_cast<unsigned long long>(pick.objectId),
                    pick.objectId == expectedId ? 1 : 0, pick.entryDistance,
                    static_cast<unsigned long long>(pick.testedCount),
                    static_cast<unsigned long long>(pick.hitCount));
            logged = true;
          };

      if (!captureWorldPickFloorLogged && haveFloorBounds) {
        const Vec3 floorTopCorner{
            floorBoxMin.x + (floorBoxMax.x - floorBoxMin.x) * 0.85F,
            floorBoxMax.y,
            floorBoxMin.z + (floorBoxMax.z - floorBoxMin.z) * 0.85F};
        logWorldPickProof("floor_overlap", floorObjectId, floorTopCorner,
                          captureWorldPickFloorLogged);
      }
      const auto logObjectCenterPick =
          [&](const char* label, creative::CreativeObjectId expectedId,
              bool& logged) {
            const creative::CreativeObject* object =
                appState.facade.findObject(expectedId);
            if (object == nullptr) {
              return;
            }
            logWorldPickProof(label, expectedId,
                              visualBoundsCenter(visualBoundsForObject(*object)),
                              logged);
          };
      logObjectCenterPick("point_proxy", captureScript.pointTargetId,
                          captureWorldPickPointLogged);
      logObjectCenterPick("line_proxy", captureScript.lineTargetId,
                          captureWorldPickLineLogged);
      logObjectCenterPick("path_proxy", captureScript.pathTargetId,
                          captureWorldPickPathLogged);
    }

    bool clickRequested = false;
    float clickX = 0.0F;
    float clickY = 0.0F;
    if (!capturePath.empty() && placeMode) {
      // Place-mode capture: no select-click is synthesized; the placement script
      // below drops objects directly. Leave clickRequested false.
    } else if (!capturePath.empty()) {
      // Synthesize a click on the FLOOR once the swapchain has settled (frame ~2),
      // aimed at a point on the floor's TOP surface near a corner — a point the
      // crate does NOT cover — so the generic nearest-hit picker selects the FLOOR
      // and not the crate. We project that single world point through the SAME
      // clipFromWorld the hit-test uses.
      if (frameIndex == 3U && haveFloorBounds) {
        // 80% out toward the +X/+Z corner of the floor top, well past the crate's
        // XZ footprint. This is only which pixel we click; the pick logic itself
        // is unchanged and object-agnostic.
        const Vec3 floorTopCorner{
            floorBoxMin.x + (floorBoxMax.x - floorBoxMin.x) * 0.85F,
            floorBoxMax.y,
            floorBoxMin.z + (floorBoxMax.z - floorBoxMin.z) * 0.85F};
        const ScreenPoint p = projectPointToScreen(
            frame.camera.clipFromWorld, floorTopCorner, extent.width,
            extent.height);
        if (p.valid) {
          clickRequested = true;
          clickX = p.x;
          clickY = p.y;
        }
      }
    } else if (!placeMode) {
      // Interactive: hold Left-Alt to release fly-look and click to select.
      // Skipped in Place mode — the interactive Place block below owns the click.
      const bool* selKeys = SDL_GetKeyboardState(nullptr);
      const bool altHeld =
          selKeys != nullptr && (selKeys[SDL_SCANCODE_LALT] != 0);
      if (altHeld) {
        window.setRelativeMouseMode(false);
        float mx = 0.0F;
        float my = 0.0F;
        const SDL_MouseButtonFlags buttons = SDL_GetMouseState(&mx, &my);
        if ((buttons & SDL_BUTTON_LMASK) != 0U) {
          clickRequested = true;
          // Window is high-DPI; scale logical mouse coords to drawable pixels.
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
          clickX = mx * scaleX;
          clickY = my * scaleY;
        }
      } else {
        window.setRelativeMouseMode(true);
      }
    }

    if (clickRequested) {
      const WorldRay ray =
          worldRayFromPixel(frame.camera, clickX, clickY, extent.width,
                            extent.height);
      const ObjectVisualPickResult pick =
          pickNearestVisualBoundsObject(objectPickCandidates, ray);
      const creative::CreativeObjectId pickedId = pick.objectId;
      SDL_Log("iggy3d_creative: WORLD_PICK click=(%.1f, %.1f) rayValid=%d "
              "tested=%llu hits=%llu pickedObjectId=%llu entryDistance=%.3f",
              clickX, clickY, pick.rayValid ? 1 : 0,
              static_cast<unsigned long long>(pick.testedCount),
              static_cast<unsigned long long>(pick.hitCount),
              static_cast<unsigned long long>(pickedId), pick.entryDistance);
      creative::CreativeToolInputPacket packet;
      packet.kind = creative::CreativeToolInputKind::PointerPress;
      packet.pointer.button = creative::CreativeToolPointerButton::Primary;
      if (pickedId != creative::kInvalidObjectId) {
        packet.pointer.target =
            creative::TargetRef{static_cast<creative::Id>(pickedId)};
      }  // A miss leaves target invalid -> Select clears selection.
      (void)appState.facade.dispatchToolInput(packet);
    }

    // ---- PLACE -------------------------------------------------------------
    // In Place mode a click drops a NEW object of the current brush kind at the
    // aimed cell, snapped to the grid, via the SAME generic createDocumentObject.
    // The new object joins the document immediately, so next frame it renders and
    // is Select/Move/Gizmo-able with ZERO extra code. There is NO per-kind place
    // branch; placeBrushObject() reads descriptor-derived footprint geometry.
    if (placeMode && capturePath.empty()) {
      // Interactive: hold Left-Alt (release fly-look) and left-click to drop at
      // the aimed cell. Edge-triggered so one click drops exactly one object.
      const bool* plKeys = SDL_GetKeyboardState(nullptr);
      const bool altHeld =
          plKeys != nullptr && (plKeys[SDL_SCANCODE_LALT] != 0);
      if (altHeld) {
        window.setRelativeMouseMode(false);
        float mx = 0.0F;
        float my = 0.0F;
        const SDL_MouseButtonFlags buttons = SDL_GetMouseState(&mx, &my);
        const bool lDown = (buttons & SDL_BUTTON_LMASK) != 0U;
        if (lDown && !placeButtonDown) {
          placeButtonDown = true;
          (void)placeBrushObjectWithUndo(appState.facade, undoStack,
                                         placeBrush, aimCellCenter,
                                         ++placedCount,
                                         "place_interactive");
        } else if (!lDown) {
          placeButtonDown = false;
        }
      } else {
        window.setRelativeMouseMode(true);
        placeButtonDown = false;
      }
    }

    // ---- CAPTURE SCENARIO (--capture) --------------------------------------
    if (!capturePath.empty()) {
      StandaloneCaptureScenarioStepRequest captureStep;
      captureStep.enabled = true;
      captureStep.frameIndex = frameIndex;
      captureStep.appState = &appState;
      captureStep.undoStack = &undoStack;
      captureStep.captureScript = &captureScript;
      captureStep.placeBrush = &placeBrush;
      captureStep.placeMode = &placeMode;
      captureStep.placedCount = &placedCount;
      captureStep.placeCellSize = placeCellSize;
      captureStep.saveRoot = &saveRoot;
      captureStep.saveId = &saveId;
      captureStep.moveHeldAxisForX = heldAxisForGrabbedAxis(GizmoAxis::X);
      captureStep.moveHeldAxisForZ = heldAxisForGrabbedAxis(GizmoAxis::Z);
      captureStep.deleteSelected = [&](std::string_view source) {
        return deleteSelectedObject(appState, source, &undoStack);
      };
      runStandaloneCaptureScenarioStep(captureStep);
    }

    // ---- RESOLVE THE SELECTION (generic) -----------------------------------
    // Everything downstream — the yellow box, the gizmo, the dimension label, and
    // the Move — keys off the CURRENTLY SELECTED object id, looked up via the same
    // findObject the inspector uses. No hardcoded crate id, no kind check. When
    // nothing is selected we draw no gizmo/box and skip Move.
    const creative::Id selectedId =
        appState.facade.selectionState().selectedTarget.value;
    const creative::CreativeObject* selected =
        selectedId != 0
            ? appState.facade.findObject(
                  static_cast<creative::CreativeObjectId>(selectedId))
            : nullptr;
    const bool hasSelection = selected != nullptr && selected->visible;
    // Selected object's world bounds (defaults are unused when !hasSelection).
    Vec3 selBoxMin{-0.5F, 0.0F, -0.5F};
    Vec3 selBoxMax{0.5F, 1.0F, 0.5F};
    if (hasSelection) {
      const VisualBounds selectedVisualBounds = visualBoundsForObject(*selected);
      selBoxMin = selectedVisualBounds.min;
      selBoxMax = selectedVisualBounds.max;
    }

    // ---- GIZMO GEOMETRY -----------------------------------------------------
    // Build the 3 axis shafts at the selected object's center C = (min+max)/2.
    // Each shaft is a single AXIS-ALIGNED world segment (start=C, end=C+dir*L),
    // which is the ONLY geometry the renderer's creativeDebugLineBox will draw
    // (it silently skips any segment moving along more than one world axis). The
    // gizmo wireframe lines are appended to the yellow selection-box lines below.
    const Vec3 gizmoCenter{(selBoxMin.x + selBoxMax.x) * 0.5F,
                           (selBoxMin.y + selBoxMax.y) * 0.5F,
                           (selBoxMin.z + selBoxMax.z) * 0.5F};
    std::array<GizmoAxisShaft, 3> gizmoShafts{};
    gizmoShafts[0] = {GizmoAxis::X,
                      {gizmoCenter.x + kGizmoAxisLength, gizmoCenter.y,
                       gizmoCenter.z},
                      {1.0F, 0.0F, 0.0F, 1.0F}};
    gizmoShafts[1] = {GizmoAxis::Y,
                      {gizmoCenter.x, gizmoCenter.y + kGizmoAxisLength,
                       gizmoCenter.z},
                      {0.0F, 1.0F, 0.0F, 1.0F}};
    gizmoShafts[2] = {GizmoAxis::Z,
                      {gizmoCenter.x, gizmoCenter.y,
                       gizmoCenter.z + kGizmoAxisLength},
                      {0.0F, 0.0F, 1.0F, 1.0F}};

    // ---- HANDLE HIT-TEST ----------------------------------------------------
    // Project C and each axis tip to pixels; a click's nearest shaft within the
    // pixel threshold names the grabbed axis. Reused by both --capture (to grab
    // the X handle) and the interactive left-press-near-a-handle path below.
    const ScreenPoint gizmoCenterScreen = projectPointToScreen(
        frame.camera.clipFromWorld, gizmoCenter, extent.width, extent.height);
    std::array<ScreenPoint, 3> gizmoTipScreen{};
    for (std::size_t i = 0; i < 3; ++i) {
      gizmoTipScreen[i] = projectPointToScreen(frame.camera.clipFromWorld,
                                               gizmoShafts[i].tip, extent.width,
                                               extent.height);
    }
    std::vector<PathPointHandleHit> pathPointHandleHits;
    const creative::CreativeObjectId selectedPathHandleObjectId =
        static_cast<creative::CreativeObjectId>(selectedId);
    const bool selectedIsPathForHandles =
        hasSelection &&
        creative::describeObject(selected->kind).shapeKind ==
            creative::CreativeObjectShapeKind::Path &&
        validPathPoints(selected->pathPoints);
    if (selectedIsPathForHandles) {
      pathPointHandleHits = buildPathPointHandleHits(
          *selected, frame.camera.clipFromWorld, extent.width, extent.height);
    }

    if (!capturePath.empty() && !captureScript.pathPointHandleLogged &&
        selectedIsPathForHandles &&
        selectedPathHandleObjectId == captureScript.pathTargetId) {
      for (const PathPointHandleHit& handle : pathPointHandleHits) {
        SDL_Log("iggy3d_creative: PATH_HANDLE hit proxy objectId=%llu "
                "pointIndex=%zu aabbValid=%d position=(%.3f, %.3f, %.3f) "
                "screen=[%.1f, %.1f..%.1f, %.1f]",
                static_cast<unsigned long long>(handle.objectId),
                handle.pointIndex, handle.aabb.valid ? 1 : 0,
                handle.position.x, handle.position.y, handle.position.z,
                handle.aabb.minX, handle.aabb.minY, handle.aabb.maxX,
                handle.aabb.maxY);
      }
      captureScript.pathPointHandleLogged = true;
    }
    // Start anchor S for an axis-constrained move = the object's corner anchor,
    // exactly as the facade captures it on BeginMove (objectCornerAnchor): for a
    // Crate (hasTransform=true) that is transform.position, NOT bounds.min. Using
    // the SAME anchor the facade holds keeps the pinned axes grid-aligned so they
    // snap to themselves; reading bounds.min instead would desync the pinned axes
    // and let the snap drag a "held" axis off S. Constrained-move worldDestination
    // is built FROM S: grabbed axis carries the dragged value, other two pinned.
    Vec3 gizmoAnchorS{gizmoCenter.x, gizmoCenter.y, gizmoCenter.z};
    if (hasSelection) {
      gizmoAnchorS = toVec3(selected->transform.position);
    }

    // ---- MOVE --------------------------------------------------------------
    // Everything below drives the kernel's GENERIC Move: setActiveTool(Move) +
    // the PRESS/MOVE/RELEASE pointer lifecycle through dispatchToolInput. The
    // facade picks the object, snaps the world destination to the grid, and
    // commits ONE Move mutation. NO per-object position math lives here, and the
    // target is ALWAYS the currently selected id — for the capture that is
    // the FLOOR, which rides the identical path the crate did in the earlier proof.
    const creative::CreativeObjectId selectedObjectId =
        static_cast<creative::CreativeObjectId>(selectedId);
    if (!capturePath.empty() && !placeMode) {
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
      if (frameIndex == 5U && hasSelection) {
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
        if (!loggedGizmoGrab) {
          SDL_Log("iggy3d_creative: GIZMO grabbed axis=%s (expected X) on "
                  "selected id=%u kind='%s'",
                  gizmoAxisName(grabbed), selectedId,
                  std::string(creative::toString(selected->kind)).c_str());
          loggedGizmoGrab = true;
        }
        const bool ok = appState.facade.setActiveTool(creative::Tool::Move);
        SDL_Log("iggy3d_creative: setActiveTool(Move) accepted=%d", ok ? 1 : 0);
        if (!loggedMoveBefore) {
          logObjectPlacement("BEFORE",
                             appState.facade.findObject(selectedObjectId));
          loggedMoveBefore = true;
        }
        creative::CreativeToolInputPacket press;
        press.kind = creative::CreativeToolInputKind::PointerPress;
        press.pointer.button = creative::CreativeToolPointerButton::Primary;
        press.pointer.target =
            creative::TargetRef{static_cast<creative::Id>(selectedObjectId)};
        const creative::CreativeFacadeToolDispatchReceipt r =
            appState.facade.dispatchToolInput(press);
        logMoveDispatch("PRESS", r);
      } else if (frameIndex == 6U) {
        creative::CreativeToolInputPacket move;
        move.kind = creative::CreativeToolInputKind::PointerMove;
        move.pointer.button = creative::CreativeToolPointerButton::Primary;
        move.pointer.hasWorldDestination = true;
        move.pointer.worldDestination = xAxisDestination;
        move.pointer.moveHeldAxis = heldAxisForGrabbedAxis(GizmoAxis::X);
        const creative::CreativeFacadeToolDispatchReceipt r =
            appState.facade.dispatchToolInput(move);
        logMoveDispatch("MOVE", r);
      } else if (frameIndex == 7U) {
        creative::CreativeToolInputPacket release;
        release.kind = creative::CreativeToolInputKind::PointerRelease;
        release.pointer.button = creative::CreativeToolPointerButton::Primary;
        release.pointer.hasWorldDestination = true;
        release.pointer.worldDestination = xAxisDestination;
        release.pointer.moveHeldAxis = heldAxisForGrabbedAxis(GizmoAxis::X);
        const creative::CreativeFacadeToolDispatchReceipt r =
            dispatchMoveReleaseWithUndo(appState, undoStack, release,
                                        selectedObjectId,
                                        "capture_move_release");
        logMoveDispatch("RELEASE", r);
        if (!loggedMoveAfter) {
          logObjectPlacement("AFTER",
                             appState.facade.findObject(selectedObjectId));
          loggedMoveAfter = true;
        }
      }
    } else if (!placeMode &&
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

        // Camera-forward ray -> Y=0 plane -> world XZ ground point (fallback).
        const Vec3 eye = frame.camera.worldEye;
        const Vec3 fwd = frame.camera.worldForward;
        creative::CreativeToolWorldPoint ground{eye.x, 0.0, eye.z};
        if (std::fabs(fwd.y) > 1.0e-4F) {
          const float t = -eye.y / fwd.y;  // eye.y + t*fwd.y == 0
          if (t > 0.0F) {
            ground.x = static_cast<double>(eye.x + fwd.x * t);
            ground.z = static_cast<double>(eye.z + fwd.z * t);
          }
        }

        const bool selectedIsPath =
            selected != nullptr &&
            creative::describeObject(selected->kind).shapeKind ==
                creative::CreativeObjectShapeKind::Path;
        if (selectedIsPath) {
          if (lDown && !interactivePathMoveActive &&
              !interactivePathPointMoveActive) {
            PathPointHandleHit handle;
            if (pickPathPointHandle(pathPointHandleHits,
                                    cursorPx,
                                    cursorPy,
                                    handle)) {
              interactivePathPointMoveActive = true;
              interactivePathPointMoveObjectId = handle.objectId;
              interactivePathPointMoveIndex = handle.pointIndex;
              interactivePathPointMoveStartGround = ground;
              SDL_Log("iggy3d_creative: PATH_HANDLE interactive move begin "
                      "objectId=%llu pointIndex=%zu ground=(%.3f, %.3f, %.3f) "
                      "position=(%.3f, %.3f, %.3f) pathPoints='%s'",
                      static_cast<unsigned long long>(handle.objectId),
                      handle.pointIndex, ground.x, ground.y, ground.z,
                      handle.position.x, handle.position.y, handle.position.z,
                      pathPointsSummary(selected->pathPoints).c_str());
            } else {
              interactivePathMoveActive = true;
              interactivePathMoveObjectId = selectedObjectId;
              interactivePathMoveStartGround = ground;
              SDL_Log("iggy3d_creative: PATH interactive move begin objectId=%llu "
                      "ground=(%.3f, %.3f, %.3f) pathPoints='%s'",
                      static_cast<unsigned long long>(selectedObjectId),
                      ground.x, ground.y, ground.z,
                      pathPointsSummary(selected->pathPoints).c_str());
            }
          } else if (!lDown && interactivePathPointMoveActive) {
            const creative::CreativeVec3 delta{
                ground.x - interactivePathPointMoveStartGround.x,
                0.0,
                ground.z - interactivePathPointMoveStartGround.z};
            const creative::CreativeDocumentMutationReceipt receipt =
                movePathPointWithUndo(appState,
                                      undoStack,
                                      interactivePathPointMoveObjectId,
                                      interactivePathPointMoveIndex,
                                      delta,
                                      "path_point_move_interactive_release");
            SDL_Log("iggy3d_creative: PATH_HANDLE interactive move release "
                    "objectId=%llu pointIndex=%zu status='%s' changed=%d "
                    "delta=(%.3f, %.3f, %.3f)",
                    static_cast<unsigned long long>(
                        interactivePathPointMoveObjectId),
                    interactivePathPointMoveIndex,
                    std::string(creative::toString(receipt.status)).c_str(),
                    receipt.changed ? 1 : 0, delta.x, delta.y, delta.z);
            interactivePathPointMoveActive = false;
            interactivePathPointMoveObjectId = creative::kInvalidObjectId;
            interactivePathPointMoveIndex = 0U;
          } else if (!lDown && interactivePathMoveActive) {
            const creative::CreativeVec3 delta{
                ground.x - interactivePathMoveStartGround.x,
                0.0,
                ground.z - interactivePathMoveStartGround.z};
            const creative::CreativeDocumentMutationReceipt receipt =
                movePathObjectWithUndo(appState,
                                       undoStack,
                                       interactivePathMoveObjectId,
                                       delta,
                                       "path_move_interactive_release");
            SDL_Log("iggy3d_creative: PATH interactive move release objectId=%llu "
                    "status='%s' changed=%d delta=(%.3f, %.3f, %.3f)",
                    static_cast<unsigned long long>(
                        interactivePathMoveObjectId),
                    std::string(creative::toString(receipt.status)).c_str(),
                    receipt.changed ? 1 : 0, delta.x, delta.y, delta.z);
            interactivePathMoveActive = false;
            interactivePathMoveObjectId = creative::kInvalidObjectId;
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
              if (interactiveGrabbedAxis == GizmoAxis::None) {
                dest = ground;  // Free ground-plane move.
                held = creative::CreativeToolMoveHeldAxis::Y;
                return;
              }
              // Screen-space shaft direction at grab time (center -> tip).
              float sdx = interactiveGrabTipScreen.x - interactiveGrabCenterScreen.x;
              float sdy = interactiveGrabTipScreen.y - interactiveGrabCenterScreen.y;
              const float slen = std::sqrt(sdx * sdx + sdy * sdy);
              float along = 0.0F;
              if (slen > 1.0e-3F) {
                sdx /= slen;
                sdy /= slen;
                const float cdx = cursorPx - interactiveGrabCursorX;
                const float cdy = cursorPy - interactiveGrabCursorY;
                // pixels moved along the shaft / pixels per shaft * world length.
                const float alongPx = cdx * sdx + cdy * sdy;
                along = (alongPx / slen) * kGizmoAxisLength;
              }
              dest.x = static_cast<double>(interactiveGrabAnchorS.x);
              dest.y = static_cast<double>(interactiveGrabAnchorS.y);
              dest.z = static_cast<double>(interactiveGrabAnchorS.z);
              switch (interactiveGrabbedAxis) {
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
              held = heldAxisForGrabbedAxis(interactiveGrabbedAxis);
            };

        if (lDown && !moveDragButtonDown) {
          moveDragButtonDown = true;
          // Grab an axis handle if the press landed near one; else free move.
          interactiveGrabbedAxis = pickGizmoAxisFromProjectedShafts(
              gizmoShafts,
              gizmoCenterScreen,
              gizmoTipScreen,
              cursorPx,
              cursorPy,
              kGizmoHandleThresholdPx);
          interactiveGrabAnchorS = gizmoAnchorS;
          interactiveGrabCursorX = cursorPx;
          interactiveGrabCursorY = cursorPy;
          interactiveGrabCenterScreen = gizmoCenterScreen;
          if (interactiveGrabbedAxis == GizmoAxis::X) {
            interactiveGrabTipScreen = gizmoTipScreen[0];
          } else if (interactiveGrabbedAxis == GizmoAxis::Y) {
            interactiveGrabTipScreen = gizmoTipScreen[1];
          } else if (interactiveGrabbedAxis == GizmoAxis::Z) {
            interactiveGrabTipScreen = gizmoTipScreen[2];
          }
          SDL_Log("iggy3d_creative: GIZMO grabbed axis=%s",
                  gizmoAxisName(interactiveGrabbedAxis));
          creative::CreativeToolInputPacket press;
          press.kind = creative::CreativeToolInputKind::PointerPress;
          press.pointer.button = creative::CreativeToolPointerButton::Primary;
          press.pointer.target =
              creative::TargetRef{static_cast<creative::Id>(selectedObjectId)};
          (void)appState.facade.dispatchToolInput(press);
        } else if (lDown && moveDragButtonDown) {
          creative::CreativeToolInputPacket move;
          move.kind = creative::CreativeToolInputKind::PointerMove;
          move.pointer.button = creative::CreativeToolPointerButton::Primary;
          move.pointer.hasWorldDestination = true;
          buildConstrainedDestination(move.pointer.worldDestination,
                                      move.pointer.moveHeldAxis);
          (void)appState.facade.dispatchToolInput(move);
        } else if (!lDown && moveDragButtonDown) {
          moveDragButtonDown = false;
          creative::CreativeToolInputPacket release;
          release.kind = creative::CreativeToolInputKind::PointerRelease;
          release.pointer.button = creative::CreativeToolPointerButton::Primary;
          release.pointer.hasWorldDestination = true;
          buildConstrainedDestination(release.pointer.worldDestination,
                                      release.pointer.moveHeldAxis);
          const creative::CreativeFacadeToolDispatchReceipt r =
              dispatchMoveReleaseWithUndo(appState, undoStack, release,
                                          selectedObjectId,
                                          "move_interactive_release");
          logMoveDispatch("RELEASE", r);
          interactiveGrabbedAxis = GizmoAxis::None;
        }
        }
      } else if (moveDragButtonDown) {
        moveDragButtonDown = false;  // Alt released mid-drag: drop the latch.
        interactiveGrabbedAxis = GizmoAxis::None;
      } else if (interactivePathMoveActive) {
        SDL_Log("iggy3d_creative: PATH interactive move cancelled objectId=%llu",
                static_cast<unsigned long long>(interactivePathMoveObjectId));
        interactivePathMoveActive = false;
        interactivePathMoveObjectId = creative::kInvalidObjectId;
      } else if (interactivePathPointMoveActive) {
        SDL_Log("iggy3d_creative: PATH_HANDLE interactive move cancelled "
                "objectId=%llu pointIndex=%zu",
                static_cast<unsigned long long>(
                    interactivePathPointMoveObjectId),
                interactivePathPointMoveIndex);
        interactivePathPointMoveActive = false;
        interactivePathPointMoveObjectId = creative::kInvalidObjectId;
        interactivePathPointMoveIndex = 0U;
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
    menuReq.frameIndex = frameIndex;
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
    if (placeMode) {
      const creative::CreativeObjectDescriptor& brushDescriptor =
          creative::describeObject(placeBrush);
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
    if (!loggedSelection) {
      loggedSelection = true;
      SDL_Log("iggy3d_creative: frame %llu submit outcome=%d reason='%s' "
              "meshes=%zu frustumInputMeshes=%zu frustumKeptMeshes=%zu "
              "frustumCulledMeshes=%zu frustumConservativeMeshes=%zu "
              "selectedTarget=%u hasSelection=%d selBoxLines=%zu "
              "pointMarkerLines=%zu lineMarkerLines=%zu pathHandleLines=%zu "
              "gizmoLines=%zu combinedWireLines=%zu uiRects=%zu glyphs=%zu",
              static_cast<unsigned long long>(frameIndex),
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

    if (maxFrames != 0U && frameIndex >= maxFrames) {
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
              static_cast<unsigned long long>(frameIndex),
              static_cast<int>(submit.outcome),
              std::string(submit.reason.code).c_str(), selectedId, selKind,
              hasSelection ? 1 : 0, documentWireLineCount,
              pointMarkerEdgeCount, lineMarkerEdgeCount,
              pathPointHandleEdgeCount,
              combinedWireLines.size() - documentWireLineCount -
                  pointMarkerEdgeCount - lineMarkerEdgeCount -
                  pathPointHandleEdgeCount,
              combinedWireLines.size(), placeMode ? 1 : 0,
              std::string(creative::toString(placeBrush)).c_str(),
              ghostEdgeCount, static_cast<unsigned long long>(placedCount),
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
