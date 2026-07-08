#include "ProductActiveSurfaceTestSupport.hpp"
#include "ProductAsciiRoomWindowTestSupport.hpp"

#include "app/frontend/FrontendState.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollision.hpp"
#include "app/iggy3d/gameplay/ProductRoomStore.hpp"
#include "app/iggy3d/menu/ActionHandlers.hpp"
#include "app/iggy3d/menu/DrawList.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"
#include "app/iggy3d/menu/InputRouter.hpp"
#include "app/iggy3d/menu/PauseUi.hpp"
#include "app/iggy3d/Operations.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/room_editor/EditingState.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"
#include "app/iggy3d/view/CreativeFlyAnchorStore.hpp"
#include "app/iggy3d/view/OpeningMenuView.hpp"
#include "app/iggy3d/world/ProductWorldTemplateOperations.hpp"
#include "app/iggy3d/window/InputFrame.hpp"
#include "app/platform/SdlWindow.hpp"
#include "app/iggy3d/automation/AutomationRoomEditing.hpp"
#include "app/input/InputActionRegistry.hpp"
#include "app/input/InputRouter.hpp"
#include "content/assets/RoomAsset.hpp"
#include "core/math/Transform3.hpp"
#include "runtime/replay/StateHash.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

using iggy3d::test::liveSurface;

bool expectNear(float actual,
                float expected,
                std::string_view message,
                float epsilon = 0.0001F) {
  return expect(std::fabs(actual - expected) < epsilon, message);
}

void markCreativeDocumentWindow(iggy3d::ProductAppWindowState& window) {
  window.inputDevice.interactionMode = iggy3d::ProductInteractionMode::Creative;
}

// Test-local convenience: openingMenuActionAt now takes the same
// ProductUiDrawListRequest the frame path builds. These tests only exercise
// hit-test geometry from a bare FrontendState (the wired buttons' rects depend
// only on frontend.saveBrowserMode), so a frontend-only request reproduces
// exactly what production feeds for those buttons.
iggy3d::OpeningMenuHitTestResult hitAt(const iggy3d::FrontendState& frontend,
                                       float x,
                                       float y) {
  iggy3d::ProductUiDrawListRequest request;
  request.frontend = &frontend;
  request.compatibleSaveCount = 1U;
  request.gameplayActive = true;
  request.saveRootWritable = true;
  request.developerToolsEnabled = true;
  request.activeRoomEditable = true;
  request.roomEditingReady = true;
  return iggy3d::openingMenuActionAt(request, x, y);
}

iggy3d::OpeningMenuHitTestResult hitStarterAction(
    const iggy3d::FrontendState& frontend,
    iggy3d::FrontendAction action) {
  constexpr float kRowX = 62.0F;
  constexpr float kRowY = 150.0F;
  constexpr float kRowStep = 52.0F;
  const std::vector<iggy3d::FrontendAction>& actions =
      iggy3d::starterActionOrder();
  for (std::size_t index = 0; index < actions.size(); ++index) {
    if (actions[index] == action) {
      return hitAt(frontend, kRowX, kRowY + static_cast<float>(index) * kRowStep);
    }
  }
  return {};
}

iggy3d::ProductAsciiRoomAuthoringRequest smallRoomRequest() {
  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceText =
      "###\n"
      "#P#\n"
      "###\n";
  request.roomId = "input_frame_room";
  request.sourceName = "unit/input_frame_room.iggyroom.txt";
  request.emitAssetText = false;
  return request;
}

iggy3d::FrontendState gameplayFrontend() {
  iggy3d::FrontendState frontend;
  iggy3d::enterFrontendGameplay(frontend, iggy3d::FrontendAction::NewWorld);
  return frontend;
}

iggy3d::FrontendState starterFrontend() {
  iggy3d::FrontendState frontend;
  frontend.screen = iggy3d::FrontendScreen::Starter;
  frontend.childScreen = iggy3d::FrontendScreen::Gameplay;
  frontend.selectedAction = iggy3d::FrontendAction::Continue;
  frontend.status = "starter_screen_ready";
  return frontend;
}

iggy3d::ProductViewportFrameConfig simpleViewportConfig() {
  iggy3d::ProductViewportFrameConfig config;
  config.centerX = 100.0F;
  config.centerY = 100.0F;
  config.pixelsPerMeter = 100.0F;
  return config;
}

iggy3d::ProductAppWindowState editingWindow() {
  iggy3d::ProductAppWindowState window;
  window.gameplay.gameplayActive = true;

  const iggy3d::ProductRoomEditingStartResult started =
      iggy3d::startProductRoomEditingFromAscii(smallRoomRequest());
  iggy3d::recordProductRoomEditingStart(window, started, "unit_edit_room");
  window.creativeAuthoring.roomEditorCursor = {};
  window.creativeAuthoring.roomEditorCursor.selectedTool = iggy3d::ProductRoomEditorTool::Wall;
  return window;
}

iggy3d::RoomSpatialSurface inputFrameClamberFloorSurface() {
  iggy3d::RoomSpatialSurface surface;
  surface.id = "floor";
  surface.sourceStaticMeshId = "floor_mesh";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Plane;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Walkable;
  surface.pointsMeters = {
      {-10.0F, 0.0F, -10.0F},
      {10.0F, 0.0F, -10.0F},
      {10.0F, 0.0F, 10.0F},
      {-10.0F, 0.0F, 10.0F},
  };
  surface.normal = {0.0F, 1.0F, 0.0F};
  surface.traversalTags = {"walkable", "clamber"};
  surface.collisionMask = {"actor"};
  return surface;
}

iggy3d::RoomSpatialSurface inputFrameClamberTopSurface() {
  iggy3d::RoomSpatialSurface surface;
  surface.id = "clamber_top";
  surface.sourceStaticMeshId = "clamber_block";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Plane;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Walkable;
  surface.pointsMeters = {
      {2.0F, 1.0F, -1.5F},
      {4.0F, 1.0F, -1.5F},
      {4.0F, 1.0F, -0.5F},
      {2.0F, 1.0F, -0.5F},
  };
  surface.normal = {0.0F, 1.0F, 0.0F};
  surface.traversalTags = {"walkable"};
  surface.collisionMask = {"actor"};
  return surface;
}

iggy3d::RoomSpatialSurface inputFrameClamberBlockerSurface() {
  iggy3d::RoomSpatialSurface surface;
  surface.id = "clamber_blocker";
  surface.sourceStaticMeshId = "clamber_block";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Box;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Blocker;
  surface.pointsMeters = {
      {2.0F, 0.0F, -1.5F},
      {4.0F, 0.0F, -1.5F},
      {4.0F, 1.0F, -0.5F},
      {2.0F, 1.0F, -0.5F},
  };
  surface.normal = {0.0F, 0.0F, 1.0F};
  surface.traversalTags = {"blocker", "clamber"};
  surface.collisionMask = {"actor"};
  surface.blocksActor = true;
  return surface;
}

iggy3d::RoomAsset inputFrameClamberRoom() {
  iggy3d::RoomAsset room;
  room.id = "input_frame_clamber_test";

  iggy3d::RoomStaticMeshAsset floor;
  floor.id = "floor_mesh";
  floor.meshId = "floor";
  floor.role = "floor";
  floor.positionMeters = {0.0F, 0.0F, 0.0F};
  floor.sizeMeters = {20.0F, 0.1F, 20.0F};
  room.staticMeshes.push_back(floor);

  iggy3d::RoomStaticMeshAsset block;
  block.id = "clamber_block";
  block.meshId = "block";
  block.role = "ledge";
  block.positionMeters = {3.0F, 0.5F, -1.0F};
  block.sizeMeters = {2.0F, 1.0F, 1.0F};
  room.staticMeshes.push_back(block);

  room.spatialSurfaces.push_back(inputFrameClamberFloorSurface());
  room.spatialSurfaces.push_back(inputFrameClamberTopSurface());
  room.spatialSurfaces.push_back(inputFrameClamberBlockerSurface());
  return room;
}

void setInputFramePlayerPosition(iggy3d::Session& session, iggy3d::Vec3 position) {
  iggy3d::SessionState& state = session.mutableStateForOwnedSystems();
  const iggy3d::EntityId actor = state.players.actorForSlot(0);
  const iggy3d::EntityState* player = state.world.findById(actor);
  if (!expect(player != nullptr, "input frame player exists for reposition")) {
    return;
  }
  iggy3d::Transform3 transform = player->transform;
  transform.position = position;
  const iggy3d::WorldMutationResult mutation =
      state.world.updateTransform(actor, transform);
  expect(mutation.status == iggy3d::WorldStatus::Ok,
         "input frame player reposition ok");
  state.currentStateHash = iggy3d::computeStateHash(state);
}

void setInputFrameClamberActiveRoom(iggy3d::ProductAppWindowState& window,
                                    const iggy3d::Session& session) {
  iggy3d::activeRoom(window).loaded = true;
  iggy3d::activeRoom(window).status = "loaded";
  iggy3d::activeRoom(window).reasonCode = "active_room_loaded";
  iggy3d::activeRoom(window).source = "unit";
  iggy3d::activeRoom(window).roomId = "input_frame_clamber_test";
  iggy3d::activeRoom(window).sourceName = "unit/input_frame_clamber_test";
  iggy3d::activeRoom(window).room = inputFrameClamberRoom();
  iggy3d::activeRoom(window).staticMeshCount = iggy3d::activeRoom(window).room.staticMeshes.size();
  iggy3d::activeRoom(window).spatialSurfaceCount =
      iggy3d::activeRoom(window).room.spatialSurfaces.size();
  iggy3d::activeRoom(window).walkableSurfaceCount = 2U;
  iggy3d::activeRoom(window).actorBlockerSurfaceCount = 1U;
  iggy3d::activeRoomCollision(window) =
      iggy3d::buildProductActiveRoomCollision(iggy3d::activeRoom(window), session.state());
}

iggy3d::ProductAppWindowState gameplayWindow(
    std::optional<iggy3d::Session>& session) {
  return iggy3d::test::activateAsciiRoomWindowForTest(
      session,
      {
          "#######\n"
          "#.....#\n"
          "#..P..#\n"
          "#.....#\n"
          "#..$.E#\n"
          "#######\n",
          "input_frame_gameplay_room",
          "unit/input_frame_gameplay_room.iggyroom.txt",
          "input frame gameplay activation ok",
          std::nullopt,
          std::nullopt,
          iggy3d::ProductInteractionMode::Player,
      });
}

iggy3d::ProductWindowEditorMousePickPreviewContext pickContext(
    const iggy3d::FrontendState& frontend,
    iggy3d::ProductAppWindowState& window,
    iggy3d::MouseClick click) {
  return {
      frontend,
      window,
      click,
      simpleViewportConfig(),
      {},
  };
}

iggy3d::MouseClick clickAt(float x, float y) {
  iggy3d::MouseClick click;
  click.clicked = true;
  click.x = x;
  click.y = y;
  return click;
}

iggy3d::ProductWindowInputClickOverride clickOverrideFor(
    iggy3d::MouseClick click) {
  iggy3d::ProductWindowInputClickOverride clickOverride;
  clickOverride.enabled = true;
  clickOverride.click = click;
  return clickOverride;
}

iggy3d::MouseClick clickPauseAction(const iggy3d::FrontendState& frontend,
                                    iggy3d::FrontendAction action) {
  iggy3d::PauseMenuContext pauseContext;
  pauseContext.pauseOpen = true;
  pauseContext.runtimeSessionAvailable = true;
  pauseContext.saveRootWritable = true;
  pauseContext.compatibleSaveCount = 1U;
  pauseContext.developerToolsEnabled = true;
  pauseContext.activeRoomEditable = true;
  pauseContext.roomEditingReady = true;
  const iggy3d::PauseMenuModel model =
      iggy3d::buildPauseMenuModel(pauseContext, frontend.selectedAction);
  iggy3d::ProductPauseUiRequest request;
  request.model = &model;
  const iggy3d::ProductUiDrawList list =
      iggy3d::buildProductPauseUiDrawList(request);

  std::string semanticId = "pause.row.";
  semanticId.append(iggy3d::frontendActionName(action));
  semanticId.append(".label");
  for (const iggy3d::UiHitRegion& region : list.hitRegions) {
    if (region.semanticId == semanticId) {
      return clickAt(region.rect.x + region.rect.width * 0.5F,
                     region.rect.y + region.rect.height * 0.5F);
    }
  }
  return {};
}

iggy3d::ProductSaveBridgeResult compatibleSaveBridge() {
  iggy3d::SaveSlotPreview slot;
  slot.id = "save_unit";
  slot.enabled = true;
  slot.compatibility = iggy3d::SaveSlotCompatibility::Compatible;
  slot.displayTitle = "Unit Save";

  iggy3d::ProductSaveBridgeResult saves;
  saves.slots.slots.push_back(slot);
  saves.slots.compatibleCount = 1U;
  return saves;
}

struct MouseDispatchHarness {
  iggy3d::FrontendState frontend = starterFrontend();
  iggy3d::ProductSaveBridgeResult saves = compatibleSaveBridge();
  iggy3d::ProductAppOptions options;
  iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::None;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::WorldSetupDraft draft =
      iggy3d::makeDefaultWorldSetupDraft("mouse_dispatch_seed");
  iggy3d::ProductAppWindowState window;
  bool closeRequested = false;
  iggy3d::FrontendSettings settings;
  iggy3d::ActionState actionState;
  iggy3d::MouseClick click = clickAt(452.0F, 508.0F);

  iggy3d::ProductOpeningMenuInputContext context() {
    return {
        frontend,
        saves,
        options,
        settingsTab,
        activeSession,
        draft,
        window,
        closeRequested,
        settings,
    };
  }
};

iggy3d::OpeningMenuHitTestResult hitArea(iggy3d::OpeningMenuHitArea area) {
  iggy3d::OpeningMenuHitTestResult hit;
  hit.hit = true;
  hit.area = area;
  return hit;
}

bool creativeClickPicksCursorAndBuildsPreviewWithoutMutation() {
  const iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window = editingWindow();
  const std::uint64_t initialFloors = window.creativeAuthoring.roomEditing.documentFloorCount;
  const std::uint64_t initialWalls = window.creativeAuthoring.roomEditing.documentWallCount;
  const std::uint64_t initialWalkable =
      window.creativeAuthoring.roomEditing.collisionWalkableSurfaceCount;
  const std::uint64_t initialBlockers =
      window.creativeAuthoring.roomEditing.collisionActorBlockerSurfaceCount;

  const iggy3d::ProductWindowEditorMousePickPreviewResult result =
      iggy3d::processProductWindowEditorMousePickPreview(
          pickContext(frontend, window, clickAt(300.0F, 100.0F)));

  return expect(result.handled, "creative mouse click handled") &&
         expect(window.inputDevice.interactionMode == iggy3d::ProductInteractionMode::Creative,
                "edit room entry keeps creative mode") &&
         expect(result.picked, "creative mouse click picked") &&
         expect(result.previewBuilt, "creative mouse click built preview") &&
         expect(result.accepted, "creative mouse click preview accepted") &&
         expect(result.status == "room_editor_preview_ready",
                "creative mouse preview status") &&
         expect(liveSurface(frontend, window).inputOwner == iggy3d::MenuOwner::Editor,
                "creative mouse input owner editor") &&
         expect(window.inputDevice.lastInputAction ==
                    iggy3d::InputAction::EditorPreviewPlacement,
                "creative mouse semantic action") &&
         expect(window.inputDevice.lastInputAccepted, "creative mouse input accepted") &&
         expect(liveSurface(frontend, window).gameplayInputSuppressed,
                "creative mouse suppresses gameplay input") &&
         expect(window.creativeAuthoring.roomEditorStatus == "room_editor_mouse_pick_mapped",
                "creative mouse pick status") &&
         expect(window.creativeAuthoring.roomEditorLastOperation == "room_editor.mouse_pick",
                "creative mouse pick operation") &&
         expect(window.creativeAuthoring.roomEditorCursor.gridX == 2,
                "creative mouse pick cursor x") &&
         expect(window.creativeAuthoring.roomEditorCursor.gridZ == 0,
                "creative mouse pick cursor z") &&
         expect(window.creativeAuthoring.roomEditorCursor.selectedTool ==
                    iggy3d::ProductRoomEditorTool::Wall,
                "creative mouse pick preserves tool") &&
         expect(window.creativeAuthoring.roomEditorPreview.visible,
                "creative mouse preview visible") &&
         expect(window.creativeAuthoring.roomEditorPreview.status == "room_editor_preview_ready",
                "creative mouse preview receipt status") &&
         expect(window.creativeAuthoring.roomEditorPreview.candidateId == "edit_wall_1",
                "creative mouse preview candidate") &&
         expect(window.creativeAuthoring.roomEditorPreview.tool == "wall",
                "creative mouse preview tool") &&
         expect(window.creativeAuthoring.roomEditorPreview.gridX == 2,
                "creative mouse preview grid x") &&
         expect(window.creativeAuthoring.roomEditorPreview.gridZ == 0,
                "creative mouse preview grid z") &&
         expect(window.creativeAuthoring.roomEditing.documentFloorCount == initialFloors,
                "creative mouse leaves floor count") &&
         expect(window.creativeAuthoring.roomEditing.documentWallCount == initialWalls,
                "creative mouse leaves wall count") &&
         expect(window.creativeAuthoring.roomEditing.collisionWalkableSurfaceCount == initialWalkable,
                "creative mouse leaves walkable collision") &&
         expect(window.creativeAuthoring.roomEditing.collisionActorBlockerSurfaceCount ==
                    initialBlockers,
                "creative mouse leaves blocker collision");
}

bool playerClickDoesNotRunEditorPickPreview() {
  const iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window = editingWindow();
  window.inputDevice.interactionMode = iggy3d::ProductInteractionMode::Player;
  const std::uint64_t initialWalls = window.creativeAuthoring.roomEditing.documentWallCount;

  const iggy3d::ProductWindowEditorMousePickPreviewResult result =
      iggy3d::processProductWindowEditorMousePickPreview(
          pickContext(frontend, window, clickAt(300.0F, 100.0F)));

  return expect(!result.handled, "player mouse click not editor handled") &&
         expect(!result.picked, "player mouse click not picked") &&
         expect(!result.previewBuilt, "player mouse click no preview") &&
         expect(!result.accepted, "player mouse click not accepted") &&
         expect(result.status == "room_editor_mouse_pick_preview_mode_blocked",
                "player mouse mode-blocked status") &&
         expect(window.creativeAuthoring.roomEditorCursor.gridX == 0,
                "player mouse leaves cursor x") &&
         expect(!window.creativeAuthoring.roomEditorPreview.visible,
                "player mouse leaves preview hidden") &&
         expect(window.creativeAuthoring.roomEditing.documentWallCount == initialWalls,
                "player mouse leaves wall count");
}

bool notReadyClickDoesNotMutateEditorState() {
  const iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window;
  window.gameplay.gameplayActive = true;
  window.inputDevice.interactionMode = iggy3d::ProductInteractionMode::Creative;

  const iggy3d::ProductWindowEditorMousePickPreviewResult result =
      iggy3d::processProductWindowEditorMousePickPreview(
          pickContext(frontend, window, clickAt(300.0F, 100.0F)));

  return expect(!result.handled, "not-ready mouse click not handled") &&
         expect(result.status == "room_editor_not_ready",
                "not-ready mouse click status") &&
         expect(!window.creativeAuthoring.roomEditing.ready, "not-ready leaves editing off") &&
         expect(!window.creativeAuthoring.roomEditorPreview.visible,
                "not-ready leaves preview hidden");
}

bool invalidClickPropagatesMousePickRejectionWithoutMutation() {
  const iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window = editingWindow();
  const std::uint64_t initialWalls = window.creativeAuthoring.roomEditing.documentWallCount;

  const iggy3d::ProductWindowEditorMousePickPreviewResult result =
      iggy3d::processProductWindowEditorMousePickPreview(
          pickContext(frontend,
                      window,
                      clickAt(std::numeric_limits<float>::infinity(), 100.0F)));

  return expect(result.handled, "invalid mouse click handled by editor") &&
         expect(!result.picked, "invalid mouse click not picked") &&
         expect(!result.previewBuilt, "invalid mouse click no preview") &&
         expect(!result.accepted, "invalid mouse click not accepted") &&
         expect(result.status == "room_editor_mouse_pick_invalid_input",
                "invalid mouse pick status") &&
         expect(window.creativeAuthoring.roomEditorStatus ==
                    "room_editor_mouse_pick_invalid_input",
                "invalid mouse pick receipt status") &&
         expect(!window.inputDevice.lastInputAccepted, "invalid mouse input not accepted") &&
         expect(!window.creativeAuthoring.roomEditorPreview.visible,
                "invalid mouse leaves preview hidden") &&
         expect(window.creativeAuthoring.roomEditing.documentWallCount == initialWalls,
                "invalid mouse leaves wall count");
}

bool controllerSouthStagesThenConfirmsPlacement() {
  const iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window = editingWindow();
  window.creativeAuthoring.roomEditorCursor.gridX = 1;
  iggy3d::Session session;
  iggy3d::ProductControllerModeChordState chord;
  iggy3d::ProductControllerActionRoutingState routing;
  const std::uint64_t initialWalls = window.creativeAuthoring.roomEditing.documentWallCount;

  const iggy3d::GamepadControllerActionSample south =
      iggy3d::productControllerActionSampleForControl(
          iggy3d::ProductControllerControl::SouthButton);
  const iggy3d::ProductControllerSampleInputResult staged =
      iggy3d::processProductControllerActionSample(
          {frontend, window, &session, chord, routing, nullptr, "unit"}, south);

  const bool firstOk =
      expect(staged.actionApplied, "first south action applied") &&
      expect(staged.actionAccepted, "first south stages accepted preview") &&
      expect(window.inputDevice.lastInputAction == iggy3d::InputAction::EditorPlace,
             "first south routes to editor place") &&
      expect(window.creativeAuthoring.roomEditorStatus == "room_editor_preview_ready",
             "first south stages preview") &&
      expect(window.creativeAuthoring.roomEditorLastOperation == "editor.place",
             "first south operation is editor place") &&
      expect(window.creativeAuthoring.roomEditorPreview.active, "first south preview pending") &&
      expect(window.creativeAuthoring.roomEditorPreview.visible, "first south preview visible") &&
      expect(window.creativeAuthoring.roomEditorPreview.candidateId == "edit_wall_1",
             "first south candidate id") &&
      expect(window.creativeAuthoring.roomEditing.documentWallCount == initialWalls,
             "first south does not mutate walls");

  (void)iggy3d::processProductControllerActionSample(
      {frontend, window, &session, chord, routing, nullptr, "unit"},
      iggy3d::GamepadControllerActionSample{});
  const iggy3d::ProductControllerSampleInputResult confirmed =
      iggy3d::processProductControllerActionSample(
          {frontend, window, &session, chord, routing, nullptr, "unit"}, south);

  return firstOk &&
         expect(confirmed.actionApplied, "second south action applied") &&
         expect(confirmed.actionAccepted, "second south confirms preview") &&
         expect(window.inputDevice.lastInputAction == iggy3d::InputAction::EditorPlace,
                "second south routes to editor place") &&
         expect(window.creativeAuthoring.roomEditorStatus == "room_editor_preview_confirmed",
                "second south confirm status") &&
         expect(window.creativeAuthoring.roomEditorLastOperation == "editor.place",
                "second south operation is editor place") &&
         expect(!window.creativeAuthoring.roomEditorPreview.active,
                "second south clears pending preview") &&
         expect(!window.creativeAuthoring.roomEditorPreview.visible,
                "second south hides preview") &&
         expect(window.creativeAuthoring.roomEditorLastPrimitiveId == "edit_wall_1",
                "second south primitive id") &&
         expect(window.creativeAuthoring.roomEditing.documentWallCount == initialWalls + 1U,
                "second south mutates walls") &&
         expect(window.creativeAuthoring.roomEditingLastOperation == "editor.place",
                "second south editing operation") &&
         expect(window.creativeAuthoring.roomEditingLastOperationAccepted,
                "second south editing accepted");
}

bool controllerMoveInvalidatesPendingPreview() {
  const iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window = editingWindow();
  iggy3d::Session session;
  iggy3d::ProductControllerModeChordState chord;
  iggy3d::ProductControllerActionRoutingState routing;
  const std::uint64_t initialWalls = window.creativeAuthoring.roomEditing.documentWallCount;

  (void)iggy3d::processProductControllerActionSample(
      {frontend, window, &session, chord, routing, nullptr, "unit"},
      iggy3d::productControllerActionSampleForControl(
          iggy3d::ProductControllerControl::SouthButton));
  (void)iggy3d::processProductControllerActionSample(
      {frontend, window, &session, chord, routing, nullptr, "unit"},
      iggy3d::GamepadControllerActionSample{});
  const iggy3d::ProductControllerSampleInputResult moved =
      iggy3d::processProductControllerActionSample(
          {frontend, window, &session, chord, routing, nullptr, "unit"},
          iggy3d::productControllerActionSampleForControl(
              iggy3d::ProductControllerControl::DpadRight));

  return expect(moved.actionApplied, "dpad move action applied") &&
         expect(moved.actionAccepted, "dpad move accepted") &&
         expect(window.inputDevice.lastInputAction == iggy3d::InputAction::EditorNudgeX,
                "dpad right routes to nudge x") &&
         expect(window.creativeAuthoring.roomEditorCursor.gridX == 1,
                "dpad right moves cursor") &&
         expect(window.creativeAuthoring.roomEditorStatus == "room_editor_cursor_moved",
                "dpad move cursor status") &&
         expect(!window.creativeAuthoring.roomEditorPreview.active,
                "dpad move clears pending preview") &&
         expect(!window.creativeAuthoring.roomEditorPreview.visible,
                "dpad move hides preview") &&
         expect(window.creativeAuthoring.roomEditing.documentWallCount == initialWalls,
                "dpad move does not mutate walls");
}

bool backCancelsPendingPreviewBeforePauseRoute() {
  iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window = editingWindow();
  const std::uint64_t initialWalls = window.creativeAuthoring.roomEditing.documentWallCount;
  window.creativeAuthoring.roomEditorCursor.gridX = 1;

  const iggy3d::ProductRoomEditorPreviewInputResult staged =
      iggy3d::applyProductRoomEditorPreviewInputAction(
          window, iggy3d::InputAction::EditorPlace);
  const bool stageOk =
      expect(staged.ok, "back test stages preview") &&
      expect(window.creativeAuthoring.roomEditorPreview.active, "back test preview active") &&
      expect(window.creativeAuthoring.roomEditing.documentWallCount == initialWalls,
             "back test stage leaves walls");

  const bool cancelled =
      iggy3d::cancelProductRoomEditorPendingPreviewFromBack(frontend, window);

  return stageOk && expect(cancelled, "back cancels pending preview") &&
         expect(liveSurface(frontend, window).inputOwner == iggy3d::MenuOwner::Editor,
                "back cancel owner editor") &&
         expect(window.inputDevice.lastInputAction == iggy3d::InputAction::EditorCancelPreview,
                "back cancel records editor cancel action") &&
         expect(window.inputDevice.lastInputAccepted, "back cancel input accepted") &&
         expect(liveSurface(frontend, window).gameplayInputSuppressed,
                "back cancel suppresses gameplay input") &&
         expect(window.creativeAuthoring.roomEditorStatus == "room_editor_preview_cancelled",
                "back cancel status") &&
         expect(window.creativeAuthoring.roomEditorLastOperation == "editor.preview_cancel",
                "back cancel operation") &&
         expect(!window.creativeAuthoring.roomEditorPreview.active,
                "back cancel clears pending preview") &&
         expect(!window.creativeAuthoring.roomEditorPreview.visible, "back cancel hides preview") &&
         expect(window.creativeAuthoring.roomEditing.documentWallCount == initialWalls,
                "back cancel leaves walls");
}

bool backWithoutPreviewFallsThroughPolicy() {
  iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window = editingWindow();
  const std::uint64_t initialWalls = window.creativeAuthoring.roomEditing.documentWallCount;

  const bool cancelled =
      iggy3d::cancelProductRoomEditorPendingPreviewFromBack(frontend, window);

  return expect(!cancelled, "back without preview not consumed") &&
         expect(window.creativeAuthoring.roomEditing.ready, "back without preview keeps editor ready") &&
         expect(!window.creativeAuthoring.roomEditorPreview.active,
                "back without preview leaves preview inactive") &&
         expect(window.creativeAuthoring.roomEditing.documentWallCount == initialWalls,
                "back without preview leaves walls");
}

bool controllerEastCancelsPendingPreviewWithoutMutation() {
  const iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window = editingWindow();
  window.creativeAuthoring.roomEditorCursor.gridX = 1;
  iggy3d::Session session;
  iggy3d::ProductControllerModeChordState chord;
  iggy3d::ProductControllerActionRoutingState routing;
  const std::uint64_t initialWalls = window.creativeAuthoring.roomEditing.documentWallCount;

  const iggy3d::ProductControllerSampleInputResult staged =
      iggy3d::processProductControllerActionSample(
          {frontend, window, &session, chord, routing, nullptr, "unit"},
          iggy3d::productControllerActionSampleForControl(
              iggy3d::ProductControllerControl::SouthButton));
  (void)iggy3d::processProductControllerActionSample(
      {frontend, window, &session, chord, routing, nullptr, "unit"},
      iggy3d::GamepadControllerActionSample{});
  const iggy3d::ProductControllerSampleInputResult cancelled =
      iggy3d::processProductControllerActionSample(
          {frontend, window, &session, chord, routing, nullptr, "unit"},
          iggy3d::productControllerActionSampleForControl(
              iggy3d::ProductControllerControl::EastButton));

  return expect(staged.actionApplied, "east cancel test stages action") &&
         expect(staged.actionAccepted, "east cancel test stages preview") &&
         expect(cancelled.actionApplied, "east cancel action applied") &&
         expect(cancelled.actionAccepted, "east cancel action accepted") &&
         expect(window.inputDevice.lastInputAction == iggy3d::InputAction::EditorCancelPreview,
                "east cancel routes to editor cancel preview") &&
         expect(window.creativeAuthoring.roomEditorStatus == "room_editor_preview_cancelled",
                "east cancel status") &&
         expect(window.creativeAuthoring.roomEditorLastOperation == "editor.preview_cancel",
                "east cancel operation") &&
         expect(!window.creativeAuthoring.roomEditorPreview.active,
                "east cancel clears pending preview") &&
         expect(!window.creativeAuthoring.roomEditorPreview.visible, "east cancel hides preview") &&
         expect(window.creativeAuthoring.roomEditing.documentWallCount == initialWalls,
                "east cancel leaves walls");
}

bool controllerSouthJumpsInGameplayPlayerMode() {
  const iggy3d::FrontendState frontend = gameplayFrontend();
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = gameplayWindow(session);
  if (!expect(session.has_value(), "controller jump session created")) {
    return false;
  }
  iggy3d::ProductControllerModeChordState chord;
  iggy3d::ProductControllerActionRoutingState routing;

  const iggy3d::ProductControllerSampleInputResult jumped =
      iggy3d::processProductControllerActionSample(
          {frontend, window, &*session, chord, routing, nullptr, "unit"},
          iggy3d::productControllerActionSampleForControl(
              iggy3d::ProductControllerControl::SouthButton));

  return expect(jumped.actionApplied, "gameplay south action applied") &&
         expect(liveSurface(frontend, window).inputOwner == iggy3d::MenuOwner::Gameplay,
                "gameplay south owner gameplay") &&
         expect(window.inputDevice.lastInputAccepted, "gameplay south input accepted") &&
         expect(!liveSurface(frontend, window).gameplayInputSuppressed,
                "gameplay south does not suppress gameplay") &&
         expect(window.inputDevice.lastInputAction == iggy3d::InputAction::PlayerJump,
                "gameplay south routes to jump") &&
         expect(window.inputDevice.controllerAction.inputAction == "game.jump",
                "gameplay south records jump input action") &&
         expect(window.gameplay.gameplayJump.requested,
                "gameplay south requests jump") &&
         expect(window.gameplay.gameplayJump.accepted, "gameplay south accepts jump") &&
         expect(window.gameplay.gameplayJump.active, "gameplay south jump remains active") &&
         expect(window.gameplay.gameplayJump.status == "airborne",
                "gameplay south jump airborne status") &&
         expect(window.gameplay.gameplayJump.reasonCode == "gameplay_jump_airborne",
                "gameplay south jump airborne reason");
}

bool controllerSouthDoesNotJumpWhenFrontendBlocksGameplay() {
  auto blockedJump = [](iggy3d::FrontendScreen screen,
                        iggy3d::FrontendScreen childScreen,
                        const char* label) {
    iggy3d::FrontendState frontend = gameplayFrontend();
    frontend.screen = screen;
    frontend.childScreen = childScreen;
    frontend.inputOwned = false;
    std::optional<iggy3d::Session> session;
    iggy3d::ProductAppWindowState window = gameplayWindow(session);
    if (!expect(session.has_value(), label)) {
      return false;
    }
    iggy3d::ProductControllerModeChordState chord;
    iggy3d::ProductControllerActionRoutingState routing;

    const iggy3d::ProductControllerSampleInputResult blocked =
        iggy3d::processProductControllerActionSample(
            {frontend, window, &*session, chord, routing, nullptr, "unit"},
            iggy3d::productControllerActionSampleForControl(
                iggy3d::ProductControllerControl::SouthButton));

    return expect(blocked.processed, label) &&
           expect(!blocked.actionApplied, label) &&
           expect(!blocked.actionAccepted, label) &&
           expect(!window.gameplay.gameplayJump.requested, label) &&
           expect(!window.gameplay.gameplayCommand.submitted, label);
  };

  return blockedJump(iggy3d::FrontendScreen::Settings,
                     iggy3d::FrontendScreen::Pause,
                     "settings blocks controller jump") &&
         blockedJump(iggy3d::FrontendScreen::DeleteConfirm,
                     iggy3d::FrontendScreen::Gameplay,
                     "delete confirm blocks controller jump");
}

bool mapMakerMovementStaysGameplayOwnedAndDoesNotPause() {
  iggy3d::FrontendState frontend = gameplayFrontend();
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = gameplayWindow(session);
  if (!expect(session.has_value(), "map maker movement session created")) {
    return false;
  }
  window.viewport.mapMakerStatus = "map_maker_enabled";
  window.viewport.mapMakerReasonCode = window.viewport.mapMakerStatus;
  window.inputDevice.interactionMode = iggy3d::ProductInteractionMode::Creative;

  const iggy3d::EntityId actor = session->state().players.actorForSlot(0);
  const iggy3d::EntityState* beforePlayer = session->state().world.findById(actor);
  if (!expect(beforePlayer != nullptr, "map maker movement player exists")) {
    return false;
  }
  const iggy3d::Vec3 beforePosition = beforePlayer->transform.position;
  iggy3d::ProductControllerModeChordState chord;
  iggy3d::ProductControllerActionRoutingState routing;

  const iggy3d::ProductControllerSampleInputResult moved =
      iggy3d::processProductControllerActionSample(
          {frontend, window, &*session, chord, routing, nullptr, "unit"},
          iggy3d::productControllerActionSampleForControl(
              iggy3d::ProductControllerControl::LeftStickUp));
  const iggy3d::EntityState* afterPlayer = session->state().world.findById(actor);
  if (!expect(afterPlayer != nullptr, "map maker movement player remains")) {
    return false;
  }

  return expect(moved.actionApplied, "map maker movement action applied") &&
         expect(moved.actionAccepted, "map maker movement action accepted") &&
         expect(frontend.screen == iggy3d::FrontendScreen::Gameplay,
                "map maker movement keeps gameplay screen") &&
         expect(frontend.screen != iggy3d::FrontendScreen::Pause,
                "map maker movement does not open pause") &&
         expect(liveSurface(frontend, window).inputOwner == iggy3d::MenuOwner::Gameplay,
                "map maker movement owner gameplay") &&
         expect(window.inputDevice.interactionMode == iggy3d::ProductInteractionMode::Creative,
                "map maker movement remains creative mode") &&
         expect(window.inputDevice.controllerAction.mode == "player",
                "map maker movement keeps controller fly mapping") &&
         expect(!liveSurface(frontend, window).gameplayInputSuppressed,
                "map maker movement does not suppress gameplay input") &&
         expect(iggy3d::productMapMakerLiveForWindow(frontend, window),
                "map maker remains live") &&
         expect(window.viewport.creativeFlyActive,
                "map maker movement activates creative fly") &&
         expect(window.viewport.creativeFlyStatus == "creative_fly_applied",
                "map maker movement applies creative fly") &&
         expect(iggy3d::productCreativeFlyAnchorAvailable(
                    window.viewport.creativeFlyAnchor),
                "map maker movement has store anchor") &&
         expect(window.viewport.creativeFlyAnchor.provenance ==
                    iggy3d::ProductCreativeFlyAnchorProvenance::FlyIntegrated,
                "map maker movement records integrated provenance") &&
         expect(window.viewport.creativeFlyAnchor.positionMeters.z <
                    beforePosition.z,
                "map maker movement advances store camera forward") &&
         expect(iggy3d::nearlyEqual(beforePosition,
                                    afterPlayer->transform.position),
                "map maker movement does not move player body") &&
         expect(!window.gameplay.gameplayCommand.submitted,
                "map maker movement does not submit player move command") &&
         expect(window.gameplay.gameplayCommand.status != "creative_fly_owns_movement",
                "map maker movement avoids pause-like owner status");
}

bool creativeDocumentSuppressesProductControllerMovement() {
  iggy3d::FrontendState frontend = gameplayFrontend();
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = gameplayWindow(session);
  if (!expect(session.has_value(),
              "creative document movement suppression session created")) {
    return false;
  }
  markCreativeDocumentWindow(window);
  window.viewport.mapMakerStatus = "map_maker_enabled";
  window.viewport.mapMakerReasonCode = window.viewport.mapMakerStatus;
  iggy3d::creative::CreativeAppState app;
  app.identity.saveId = "creative_save";
  app.identity.worldId = "creative_world";
  app.identity.documentId = 42U;

  const iggy3d::EntityId actor = session->state().players.actorForSlot(0);
  const iggy3d::EntityState* beforePlayer =
      session->state().world.findById(actor);
  if (!expect(beforePlayer != nullptr,
              "creative document movement player exists")) {
    return false;
  }
  const iggy3d::Vec3 beforePosition = beforePlayer->transform.position;
  iggy3d::ProductControllerModeChordState chord;
  iggy3d::ProductControllerActionRoutingState routing;

  const iggy3d::ProductControllerSampleInputResult blocked =
      iggy3d::processProductControllerActionSample(
          {frontend, window, &*session, chord, routing, nullptr, "unit", &app},
          iggy3d::productControllerActionSampleForControl(
              iggy3d::ProductControllerControl::LeftStickUp));
  const iggy3d::EntityState* afterPlayer =
      session->state().world.findById(actor);
  if (!expect(afterPlayer != nullptr,
              "creative document movement player remains")) {
    return false;
  }

  return expect(blocked.processed,
                "creative document controller sample processed") &&
         expect(!blocked.actionApplied,
                "creative document movement not applied") &&
         expect(!blocked.actionAccepted,
                "creative document movement not accepted") &&
         expect(blocked.status ==
                    "controller_sample_creative_document_suppressed",
                "creative document suppression status") &&
         expect(!iggy3d::productMapMakerLiveForSource(frontend, window, &app),
                "creative document source is not legacy map maker") &&
         expect(iggy3d::productCreativeSurfaceKindForSource(frontend,
                                                            window,
                                                            &app) ==
                    iggy3d::ProductCreativeSurfaceKind::CreativeDocument,
                "creative document source surface kind") &&
         expect(!window.viewport.creativeFlyActive,
                "creative document does not run creative fly") &&
         expect(!window.gameplay.gameplayCommand.submitted,
                "creative document does not submit gameplay command") &&
         expect(iggy3d::nearlyEqual(beforePosition,
                                    afterPlayer->transform.position),
                "creative document leaves runtime player position unchanged");
}

bool controllerChordToggleRecordsCreativeConsumption() {
  iggy3d::FrontendState frontend = gameplayFrontend();
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = gameplayWindow(session);
  if (!expect(session.has_value(), "chord toggle session created")) {
    return false;
  }

  iggy3d::ProductControllerModeChordState chord;
  iggy3d::ProductControllerActionRoutingState routing;
  const iggy3d::ProductControllerSampleInputResult toggled =
      iggy3d::processProductControllerActionSample(
          {frontend, window, &*session, chord, routing, nullptr, "unit"},
          iggy3d::productControllerModeChordActionSample());

  return expect(toggled.processed, "chord toggle processed") &&
         expect(!toggled.actionApplied, "chord toggle emits no gameplay action") &&
         expect(window.inputDevice.interactionMode == iggy3d::ProductInteractionMode::Creative,
                "chord toggle enters creative mode") &&
         expect(window.inputDevice.controllerModeToggle.requested,
                "chord toggle requested") &&
         expect(window.inputDevice.controllerModeToggle.accepted,
                "chord toggle accepted") &&
         expect(window.inputDevice.controllerModeToggle.status == "interaction_mode_toggled",
                "chord toggle status") &&
         expect(window.inputDevice.controllerAction.status ==
                    "controller_action_chord_consumed",
                "chord toggle consumes controller action") &&
         expect(window.inputDevice.controllerAction.mode == "creative",
                "chord toggle records post-toggle mode") &&
         expect(window.inputDevice.controllerAction.surface == "gameplay",
                "chord toggle surface gameplay") &&
         expect(window.inputDevice.controllerAction.inputAction == "none",
                "chord toggle no mapped action");
}

bool controllerSouthJumpsFromClamberedWallTop() {
  const iggy3d::FrontendState frontend = gameplayFrontend();
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = gameplayWindow(session);
  if (!expect(session.has_value(), "clamber jump session created")) {
    return false;
  }
  setInputFramePlayerPosition(*session, {3.0F, 0.0F, 0.10F});
  setInputFrameClamberActiveRoom(window, *session);
  window.viewport.cameraYawDegrees = 0.0F;
  iggy3d::ProductControllerModeChordState chord;
  iggy3d::ProductControllerActionRoutingState routing;
  const iggy3d::GamepadControllerActionSample south =
      iggy3d::productControllerActionSampleForControl(
          iggy3d::ProductControllerControl::SouthButton);

  const iggy3d::ProductControllerSampleInputResult clambered =
      iggy3d::processProductControllerActionSample(
          {frontend, window, &*session, chord, routing, nullptr, "unit"}, south);
  const float clamberTopY = session->state()
                                .world.findById(session->state().players.actorForSlot(0))
                                ->transform.position.y;
  (void)iggy3d::processProductControllerActionSample(
      {frontend, window, &*session, chord, routing, nullptr, "unit"},
      iggy3d::GamepadControllerActionSample{});
  const iggy3d::ProductControllerSampleInputResult jumped =
      iggy3d::processProductControllerActionSample(
          {frontend, window, &*session, chord, routing, nullptr, "unit"}, south);
  const float finalY = session->state()
                           .world.findById(session->state().players.actorForSlot(0))
                           ->transform.position.y;

  return expect(clambered.actionApplied, "wall top first south action applied") &&
         expect(window.inputDevice.controllerAction.inputAction == "game.jump",
                "wall top second south records jump action") &&
         expect(window.inputDevice.lastInputAction == iggy3d::InputAction::PlayerJump,
                "wall top second south routes to jump") &&
         expect(jumped.actionApplied, "wall top second south action applied") &&
         expect(window.gameplay.gameplayJump.requested, "wall top jump requested") &&
         expect(window.gameplay.gameplayJump.accepted, "wall top jump accepted") &&
         expect(window.gameplay.gameplayJump.active, "wall top jump active") &&
         expect(window.gameplay.gameplayJump.status == "airborne",
                "wall top jump airborne") &&
         expect(window.gameplay.gameplayJump.reasonCode == "gameplay_jump_airborne",
                "wall top jump reason") &&
         expect(window.gameplay.gameplayJump.groundY == clamberTopY,
                "wall top jump uses clamber top as ground") &&
         expect(finalY > clamberTopY, "wall top jump raises player");
}

bool starterDeleteButtonOpensSelectableBrowser() {
  iggy3d::FrontendState frontend = starterFrontend();
  frontend.selectedAction = iggy3d::FrontendAction::Delete;
  iggy3d::ProductAppOptions options;
  iggy3d::ProductSaveBridgeResult saves = compatibleSaveBridge();
  iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::None;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::WorldSetupDraft draft;
  iggy3d::ProductAppWindowState window;
  bool closeRequested = false;

  const iggy3d::ProductMenuActionResult result =
      iggy3d::applyProductStarterMenuAction(
          iggy3d::InputAction::MenuConfirm,
          {frontend,
           options,
           saves,
           settingsTab,
           activeSession,
           draft,
           window,
           closeRequested});

  // The main-menu "Delete" entry now opens the selectable save browser in
  // Delete mode (so the operator picks WHICH map), rather than jumping
  // straight to the confirm dialog. The slot list is pre-selected to the
  // first save; no delete fires until the operator confirms a chosen slot.
  return expect(result.handled, "starter delete handled") &&
         expect(result.accepted, "starter delete accepted") &&
         expect(frontend.childScreen == iggy3d::FrontendScreen::LoadSave,
                "starter delete opens selectable browser") &&
         expect(frontend.selectedAction == iggy3d::FrontendAction::Delete,
                "starter delete selected action is delete") &&
         expect(!window.saveSession.saveDelete.confirmationOpen,
                "starter delete does not auto-open confirmation") &&
         expect(window.saveSession.selectedProductSave.id == "save_unit",
                "starter delete pre-selects first save") &&
         expect(!closeRequested, "starter delete does not close app");
}

bool starterHitTestUsesCanonicalActionRows() {
  const iggy3d::FrontendState frontend = starterFrontend();
  const iggy3d::OpeningMenuHitTestResult deleteHit =
      hitStarterAction(frontend, iggy3d::FrontendAction::Delete);
  const iggy3d::OpeningMenuHitTestResult exitHit =
      hitStarterAction(frontend, iggy3d::FrontendAction::Exit);

  return expect(deleteHit.hit, "delete starter row hit") &&
         expect(deleteHit.area == iggy3d::OpeningMenuHitArea::StarterAction,
                "delete starter hit area") &&
         expect(deleteHit.action == iggy3d::FrontendAction::Delete,
                "delete starter hit action") &&
         expect(exitHit.hit, "exit starter row hit") &&
         expect(exitHit.area == iggy3d::OpeningMenuHitArea::StarterAction,
                "exit starter hit area") &&
         expect(exitHit.action == iggy3d::FrontendAction::Exit,
                "exit starter hit action");
}

bool pauseHitTestUsesPauseActionRows() {
  iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window;
  window.gameplay.gameplayActive = true;
  bool closeRequested = false;
  const iggy3d::ProductMenuActionResult paused =
      iggy3d::applyProductSystemPauseMenuAction(
          iggy3d::InputAction::SystemPause,
          {frontend, window, closeRequested});

  const iggy3d::MouseClick resumeClick =
      clickPauseAction(frontend, iggy3d::FrontendAction::Resume);
  const iggy3d::MouseClick settingsClick =
      clickPauseAction(frontend, iggy3d::FrontendAction::Settings);
  const iggy3d::OpeningMenuHitTestResult resumeHit =
      hitAt(frontend, resumeClick.x, resumeClick.y);
  const iggy3d::OpeningMenuHitTestResult settingsHit =
      hitAt(frontend, settingsClick.x, settingsClick.y);
  const iggy3d::OpeningMenuHitTestResult staleLeftHit =
      hitAt(frontend, 62.0F, 150.0F);

  return expect(paused.handled, "system pause handled") &&
         expect(paused.accepted, "system pause accepted") &&
         expect(frontend.screen == iggy3d::FrontendScreen::Pause,
                "system pause opens pause screen") &&
         expect(resumeClick.clicked, "pause resume test click found") &&
         expect(resumeHit.hit, "pause resume row hit") &&
         expect(resumeHit.action == iggy3d::FrontendAction::Resume,
                "pause first row is resume") &&
         expect(settingsClick.clicked, "pause settings test click found") &&
         expect(settingsHit.hit, "pause settings row hit") &&
         expect(settingsHit.area == iggy3d::OpeningMenuHitArea::StarterAction,
                "pause settings row uses menu action area") &&
         expect(settingsHit.action == iggy3d::FrontendAction::Settings,
                "pause settings row maps to settings") &&
         expect(!staleLeftHit.hit,
                "pause no longer uses stale left-side action rows");
}

bool pauseMouseClickResumesBeforeCreativeOverlayInput() {
  iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window;
  window.gameplay.gameplayActive = true;
  window.inputDevice.interactionMode = iggy3d::ProductInteractionMode::Creative;
  bool closeRequested = false;
  (void)iggy3d::applyProductSystemPauseMenuAction(
      iggy3d::InputAction::SystemPause, {frontend, window, closeRequested});
  const iggy3d::MouseClick resumeClick =
      clickPauseAction(frontend, iggy3d::FrontendAction::Resume);

  iggy3d::ProductUiDrawList creativeUi;
  creativeUi.ready = true;
  creativeUi.virtualWidth = 1280U;
  creativeUi.virtualHeight = 720U;
  creativeUi.hitRegions.push_back(
      iggy3d::UiHitRegion{.semanticId = "creative.row.create.create_room",
                           .rect = {0.0F, 0.0F, 1280.0F, 720.0F},
                           .kind = iggy3d::UiHitKind::Button,
                           .action = iggy3d::FrontendAction::None,
                           .enabled = true});

  iggy3d::ProductSaveBridgeResult saves = compatibleSaveBridge();
  iggy3d::ProductAppOptions options;
  options.saveRoot = std::filesystem::temp_directory_path();
  iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::None;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::WorldSetupDraft draft;
  iggy3d::FrontendSettings settings;
  iggy3d::ProductWindowInputFrameState inputFrame;

  iggy3d::processProductWindowInputFrame(
      iggy3d::ProductWindowInputFrameContext{
          frontend,
          saves,
          options,
          settingsTab,
          activeSession,
          draft,
          window,
          settings,
          inputFrame,
          closeRequested,
          nullptr,
          nullptr,
          &creativeUi,
          {},
          {},
          0,
          iggy3d::creative::CreativeViewportPickDepthMode::FixedZ,
          clickOverrideFor(resumeClick),
      });

  return expect(resumeClick.clicked, "pause resume process click found") &&
         expect(frontend.screen == iggy3d::FrontendScreen::Gameplay,
                "pause click resumes gameplay") &&
         expect(window.creativeAuthoring.creativeUiInput.requested,
                "creative ui input receipt recorded") &&
         expect(!window.creativeAuthoring.creativeUiInput.clickPresent,
                "pause-owned click is not routed to creative ui") &&
         expect(!window.creativeAuthoring.creativeUiInput.consumed,
                "pause-owned click is not consumed by creative ui") &&
         expect(window.creativeAuthoring.creativeUiInput.status ==
                    "product_creative_ui_input_no_click",
                "creative ui records no click behind pause") &&
         expect(window.creativeAuthoring.creativeUiCommand.kind == "none",
                "creative command does not fire behind pause") &&
         expect(window.creativeAuthoring.creativeUiInput.downstreamClickSuppressed,
                "pause-owned click suppresses downstream creative/gameplay click") &&
         expect(window.creativeAuthoring.creativeUiInput.downstreamClickStatus ==
                    "product_creative_ui_downstream_click_higher_priority",
                "downstream receipt names higher-priority ui");
}

// Regression (BG-1029): the creative-editor guard on the legacy opening-menu
// hit-test must NOT swallow pause-menu mouse clicks in an ACTIVE creative world.
// The prior guard skipped the block whenever the creative editor was active,
// but a paused creative world keeps interactionMode==Creative, so the pause menu
// (still drawn) became mouse-dead. Here the world is truly active
// (Creative mode set) AND paused; the Resume click must still route.
bool pauseMouseClickResumesInActiveCreativeWorld() {
  iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window;
  window.gameplay.gameplayActive = true;
  markCreativeDocumentWindow(window);
  bool closeRequested = false;
  (void)iggy3d::applyProductSystemPauseMenuAction(
      iggy3d::InputAction::SystemPause, {frontend, window, closeRequested});
  const iggy3d::MouseClick resumeClick =
      clickPauseAction(frontend, iggy3d::FrontendAction::Resume);

  iggy3d::ProductUiDrawList creativeUi;
  creativeUi.ready = true;
  creativeUi.virtualWidth = 1280U;
  creativeUi.virtualHeight = 720U;
  creativeUi.hitRegions.push_back(
      iggy3d::UiHitRegion{.semanticId = "creative.row.create.create_room",
                           .rect = {0.0F, 0.0F, 1280.0F, 720.0F},
                           .kind = iggy3d::UiHitKind::Button,
                           .action = iggy3d::FrontendAction::None,
                           .enabled = true});

  iggy3d::ProductSaveBridgeResult saves = compatibleSaveBridge();
  iggy3d::ProductAppOptions options;
  options.saveRoot = std::filesystem::temp_directory_path();
  iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::None;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::WorldSetupDraft draft;
  iggy3d::FrontendSettings settings;
  iggy3d::ProductWindowInputFrameState inputFrame;

  iggy3d::processProductWindowInputFrame(
      iggy3d::ProductWindowInputFrameContext{
          frontend,
          saves,
          options,
          settingsTab,
          activeSession,
          draft,
          window,
          settings,
          inputFrame,
          closeRequested,
          nullptr,
          nullptr,
          &creativeUi,
          {},
          {},
          0,
          iggy3d::creative::CreativeViewportPickDepthMode::FixedZ,
          clickOverrideFor(resumeClick),
      });

  return expect(resumeClick.clicked, "active-world pause resume click found") &&
         expect(frontend.screen == iggy3d::FrontendScreen::Gameplay,
                "active-world pause click resumes gameplay") &&
         expect(!window.creativeAuthoring.creativeUiInput.consumed,
                "active-world pause click not consumed by creative ui");
}

bool pauseSettingsConfirmOpensSettingsPanel() {
  iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window;
  window.gameplay.gameplayActive = true;
  bool closeRequested = false;
  iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::None;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppOptions options;
  iggy3d::ProductSaveBridgeResult saves;
  iggy3d::FrontendSettings settings;

  (void)iggy3d::applyProductSystemPauseMenuAction(
      iggy3d::InputAction::SystemPause, {frontend, window, closeRequested});
  frontend.selectedAction = iggy3d::FrontendAction::Settings;
  const iggy3d::ProductMenuActionResult opened =
      iggy3d::applyProductPauseMenuAction(
          iggy3d::InputAction::MenuConfirm,
          {frontend, options, saves, settingsTab, activeSession, window,
           closeRequested, settings});

  return expect(opened.handled, "pause settings confirm handled") &&
         expect(opened.accepted, "pause settings confirm accepted") &&
         expect(frontend.screen == iggy3d::FrontendScreen::Settings,
                "pause settings confirm opens settings screen") &&
         expect(frontend.childScreen == iggy3d::FrontendScreen::Pause,
                "pause settings preserves pause parent") &&
         expect(iggy3d::openingMenuDetailSurfaceFor(frontend) ==
                    iggy3d::ProductFrontendSurface::Settings,
                "pause settings renders settings detail surface") &&
         expect(settingsTab == iggy3d::FrontendSettingsTab::Input,
                "pause settings starts on input tab") &&
         expect(liveSurface(frontend, window).inputOwner == iggy3d::MenuOwner::Settings,
                "pause settings input owner");
}

bool pauseSettingsInputDispatchRoutesToSettings() {
  iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window;
  window.gameplay.gameplayActive = true;
  bool closeRequested = false;
  iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::None;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppOptions options;
  iggy3d::ProductSaveBridgeResult saves = compatibleSaveBridge();
  iggy3d::WorldSetupDraft draft;
  iggy3d::ActionState actionState;
  iggy3d::FrontendSettings settings;

  (void)iggy3d::applyProductSystemPauseMenuAction(
      iggy3d::InputAction::SystemPause, {frontend, window, closeRequested});
  frontend.selectedAction = iggy3d::FrontendAction::Settings;
  (void)iggy3d::applyProductPauseMenuAction(
      iggy3d::InputAction::MenuConfirm,
      {frontend, options, saves, settingsTab, activeSession, window,
       closeRequested, settings});

  const iggy3d::FrontendAction selectedBefore = frontend.selectedAction;
  iggy3d::routeProductOpeningMenuInput(
      iggy3d::InputAction::MenuDown,
      actionState,
      {frontend,
       saves,
       options,
       settingsTab,
       activeSession,
       draft,
       window,
       closeRequested,
       settings});

  return expect(settingsTab == iggy3d::FrontendSettingsTab::Controls,
                "pause settings input dispatch advances settings tab") &&
         expect(frontend.selectedAction == selectedBefore,
                "pause settings input dispatch does not move starter row") &&
         expect(liveSurface(frontend, window).inputOwner == iggy3d::MenuOwner::Settings,
                "pause settings input dispatch owner") &&
         expect(window.inputDevice.lastInputAccepted,
                "pause settings input dispatch accepted");
}

bool starterSettingsInputDispatchRoutesToSettings() {
  iggy3d::FrontendState frontend = starterFrontend();
  frontend.childScreen = iggy3d::FrontendScreen::Settings;
  const iggy3d::FrontendAction selectedBefore = frontend.selectedAction;
  iggy3d::ProductAppWindowState window;
  bool closeRequested = false;
  iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::Input;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppOptions options;
  iggy3d::ProductSaveBridgeResult saves = compatibleSaveBridge();
  iggy3d::WorldSetupDraft draft;
  iggy3d::ActionState actionState;
  iggy3d::FrontendSettings settings;

  iggy3d::routeProductOpeningMenuInput(
      iggy3d::InputAction::MenuDown,
      actionState,
      {frontend,
       saves,
       options,
       settingsTab,
       activeSession,
       draft,
       window,
       closeRequested,
       settings});

  return expect(settingsTab == iggy3d::FrontendSettingsTab::Controls,
                "starter settings dispatch advances settings tab") &&
         expect(frontend.selectedAction == selectedBefore,
                "starter settings dispatch does not move starter row") &&
         expect(liveSurface(frontend, window).inputOwner == iggy3d::MenuOwner::Settings,
                "starter settings dispatch owner") &&
         expect(window.inputDevice.lastInputAccepted, "starter settings dispatch accepted");
}

bool pauseInputDispatchRoutesToPauseHandler() {
  iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window;
  window.gameplay.gameplayActive = true;
  bool closeRequested = false;
  iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::None;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppOptions options;
  iggy3d::ProductSaveBridgeResult saves = compatibleSaveBridge();
  iggy3d::WorldSetupDraft draft;
  iggy3d::ActionState actionState;
  iggy3d::FrontendSettings settings;

  (void)iggy3d::applyProductSystemPauseMenuAction(
      iggy3d::InputAction::SystemPause, {frontend, window, closeRequested});
  iggy3d::routeProductOpeningMenuInput(
      iggy3d::InputAction::MenuDown,
      actionState,
      {frontend,
       saves,
       options,
       settingsTab,
       activeSession,
       draft,
       window,
       closeRequested,
       settings});

  return expect(frontend.selectedAction == iggy3d::FrontendAction::EditRoom,
                "pause dispatch advances pause selection") &&
         expect(frontend.status == "pause_menu_selection_changed",
                "pause dispatch status") &&
         expect(liveSurface(frontend, window).inputOwner == iggy3d::MenuOwner::Pause,
                "pause dispatch owner");
}

bool devOverlayInputDispatchRoutesToDevToolsHandler() {
  iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window;
  window.gameplay.gameplayActive = true;
  bool closeRequested = false;
  iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::None;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppOptions options;
  iggy3d::ProductSaveBridgeResult saves = compatibleSaveBridge();
  iggy3d::WorldSetupDraft draft;
  iggy3d::ActionState actionState;
  iggy3d::FrontendSettings settings;

  (void)iggy3d::applyProductSystemPauseMenuAction(
      iggy3d::InputAction::DevToggle, {frontend, window, closeRequested});
  iggy3d::routeProductOpeningMenuInput(
      iggy3d::InputAction::MenuDown,
      actionState,
      {frontend,
       saves,
       options,
       settingsTab,
       activeSession,
       draft,
       window,
       closeRequested,
       settings});

  return expect(frontend.devToolsCategory ==
                    iggy3d::FrontendDevToolsCategory::Input,
                "dev overlay dispatch advances devtools category") &&
         expect(frontend.status == "dev_overlay_selection_changed",
                "dev overlay dispatch status") &&
         expect(liveSurface(frontend, window).inputOwner == iggy3d::MenuOwner::DevTools,
                "dev overlay dispatch owner");
}

bool confirmDialogDispatchDoesNotFallThroughToStarter() {
  {
    iggy3d::FrontendState frontend = starterFrontend();
    frontend.childScreen = iggy3d::FrontendScreen::DeleteConfirm;
    frontend.selectedAction = iggy3d::FrontendAction::Continue;
    iggy3d::ProductAppWindowState window;
    window.saveSession.saveDelete.confirmationOpen = true;
    bool closeRequested = false;
    iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::None;
    std::optional<iggy3d::Session> activeSession;
    iggy3d::ProductAppOptions options;
    iggy3d::ProductSaveBridgeResult saves = compatibleSaveBridge();
    iggy3d::WorldSetupDraft draft;
    iggy3d::ActionState actionState;
    iggy3d::FrontendSettings settings;

    iggy3d::routeProductOpeningMenuInput(
        iggy3d::InputAction::MenuBack,
        actionState,
        {frontend,
         saves,
         options,
         settingsTab,
         activeSession,
         draft,
         window,
         closeRequested,
         settings});
    const bool deleteOk =
        expect(frontend.childScreen == iggy3d::FrontendScreen::LoadSave,
               "delete confirm back returns to save browser") &&
        expect(!window.saveSession.saveDelete.confirmationOpen,
               "delete confirm back cancels delete") &&
        expect(frontend.status == "save_delete_cancelled",
               "delete confirm back status") &&
        expect(!activeSession.has_value(),
               "delete confirm back does not launch starter continue");
    if (!deleteOk) {
      return false;
    }
  }

  iggy3d::FrontendState frontend = starterFrontend();
  frontend.childScreen = iggy3d::FrontendScreen::ExitConfirm;
  frontend.selectedAction = iggy3d::FrontendAction::Continue;
  iggy3d::ProductAppWindowState window;
  bool closeRequested = false;
  iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::None;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppOptions options;
  iggy3d::ProductSaveBridgeResult saves = compatibleSaveBridge();
  iggy3d::WorldSetupDraft draft;
  iggy3d::ActionState actionState;
  iggy3d::FrontendSettings settings;

  iggy3d::routeProductOpeningMenuInput(
      iggy3d::InputAction::MenuConfirm,
      actionState,
      {frontend,
       saves,
       options,
       settingsTab,
       activeSession,
       draft,
       window,
       closeRequested,
       settings});

  return expect(closeRequested, "exit confirm requests close") &&
         expect(frontend.status == "opening_menu_exit_requested",
                "exit confirm status") &&
         expect(!activeSession.has_value(),
                "exit confirm does not launch starter continue");
}

bool gameplayAndEditorSurfacesDoNotFallThroughToStarter() {
  {
    iggy3d::FrontendState frontend = gameplayFrontend();
    frontend.selectedAction = iggy3d::FrontendAction::Continue;
    iggy3d::ProductAppWindowState window;
    window.gameplay.gameplayActive = true;
    bool closeRequested = false;
    iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::None;
    std::optional<iggy3d::Session> activeSession;
    iggy3d::ProductAppOptions options;
    iggy3d::ProductSaveBridgeResult saves = compatibleSaveBridge();
    iggy3d::WorldSetupDraft draft;
    iggy3d::ActionState actionState;
    iggy3d::FrontendSettings settings;

    iggy3d::routeProductOpeningMenuInput(
        iggy3d::InputAction::MenuDown,
        actionState,
        {frontend,
         saves,
         options,
         settingsTab,
         activeSession,
         draft,
         window,
         closeRequested,
         settings});
    const bool gameplayOk =
        expect(frontend.selectedAction == iggy3d::FrontendAction::Continue,
               "gameplay menu action does not move starter selection") &&
        expect(!activeSession.has_value(),
               "gameplay menu action does not launch starter flow");
    if (!gameplayOk) {
      return false;
    }
  }

  iggy3d::FrontendState frontend = gameplayFrontend();
  frontend.selectedAction = iggy3d::FrontendAction::Continue;
  iggy3d::ProductAppWindowState window = editingWindow();
  bool closeRequested = false;
  iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::None;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppOptions options;
  iggy3d::ProductSaveBridgeResult saves = compatibleSaveBridge();
  iggy3d::WorldSetupDraft draft;
  iggy3d::ActionState actionState;
  iggy3d::FrontendSettings settings;

  iggy3d::routeProductOpeningMenuInput(
      iggy3d::InputAction::MenuDown,
      actionState,
      {frontend,
       saves,
       options,
       settingsTab,
       activeSession,
       draft,
       window,
       closeRequested,
       settings});

  return expect(frontend.selectedAction == iggy3d::FrontendAction::Continue,
                "editor menu action does not move starter selection") &&
         expect(!activeSession.has_value(),
                "editor menu action does not launch starter flow");
}

bool editorBackOpensPauseWithoutLeavingEditor() {
  iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window = editingWindow();
  bool closeRequested = false;
  iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::None;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppOptions options;
  iggy3d::ProductSaveBridgeResult saves = compatibleSaveBridge();
  iggy3d::WorldSetupDraft draft;
  iggy3d::ActionState actionState;
  iggy3d::FrontendSettings settings;

  iggy3d::routeProductOpeningMenuInput(
      iggy3d::InputAction::MenuBack,
      actionState,
      {frontend,
       saves,
       options,
       settingsTab,
       activeSession,
       draft,
       window,
       closeRequested,
       settings});

  return expect(frontend.screen == iggy3d::FrontendScreen::Pause,
                "editor back opens pause") &&
         expect(frontend.selectedAction == iggy3d::FrontendAction::Resume,
                "editor back starts pause on resume") &&
         expect(window.creativeAuthoring.roomEditing.ready,
                "editor back keeps room editing ready for leave action") &&
         expect(liveSurface(frontend, window).inputOwner == iggy3d::MenuOwner::Pause,
                "editor back routes input owner to pause") &&
         expect(window.inputDevice.lastInputAction == iggy3d::InputAction::SystemPause,
                "editor back records system pause action") &&
         expect(window.inputDevice.lastInputAccepted,
                "editor back system pause accepted");
}

bool pauseSaveFlowLeavesStableFrontendStatus() {
  iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window;
  window.gameplay.gameplayActive = true;
  bool closeRequested = false;
  iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::None;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppOptions options;
  iggy3d::ProductSaveBridgeResult saves;
  iggy3d::FrontendSettings settings;

  (void)iggy3d::applyProductSystemPauseMenuAction(
      iggy3d::InputAction::SystemPause, {frontend, window, closeRequested});
  frontend.selectedAction = iggy3d::FrontendAction::Save;
  const iggy3d::ProductMenuActionResult saved =
      iggy3d::applyProductPauseMenuAction(
          iggy3d::InputAction::MenuConfirm,
          {frontend, options, saves, settingsTab, activeSession, window, closeRequested,
           settings});

  return expect(saved.handled, "pause save handled") &&
         expect(saved.accepted, "pause save accepted") &&
         expect(frontend.status == "pause_save_failed",
                "pause save failure status remains stable") &&
         expect(window.frontendShell.launchStatus == "product_save_session_missing",
                "pause save failure launch status");
}

// sd3: selecting Load from the pause menu opens the save browser as a Pause-owned child in
// Load mode — the enum AND the string mirror set explicitly, even over a stale Delete residue.
bool pauseLoadOpensBrowserInLoadModeOverStaleResidue() {
  iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window;
  window.gameplay.gameplayActive = true;
  bool closeRequested = false;
  iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::None;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppOptions options;
  iggy3d::ProductSaveBridgeResult saves = compatibleSaveBridge();
  iggy3d::FrontendSettings settings;

  (void)iggy3d::applyProductSystemPauseMenuAction(
      iggy3d::InputAction::SystemPause, {frontend, window, closeRequested});
  // Seed a STALE Delete residue that a correct opener must overwrite.
  frontend.saveBrowserMode = iggy3d::FrontendSaveBrowserMode::Delete;
  window.saveSession.saveSlotBrowserMode = "delete";
  frontend.selectedAction = iggy3d::FrontendAction::LoadSave;
  const iggy3d::ProductMenuActionResult opened =
      iggy3d::applyProductPauseMenuAction(
          iggy3d::InputAction::MenuConfirm,
          {frontend, options, saves, settingsTab, activeSession, window,
           closeRequested, settings});

  return expect(opened.handled && opened.accepted, "pause load handled+accepted") &&
         expect(frontend.childScreen == iggy3d::FrontendScreen::LoadSave,
                "pause load opens the browser overlay") &&
         expect(frontend.screen == iggy3d::FrontendScreen::Pause,
                "pause load keeps the pause surface (Pause-owned child)") &&
         expect(frontend.saveBrowserMode == iggy3d::FrontendSaveBrowserMode::Load,
                "pause load sets Load mode over the stale Delete") &&
         expect(window.saveSession.saveSlotBrowserMode == "load",
                "pause load mirrors the mode string in lockstep");
}

// sd3: BACK from a pause-opened browser returns to the PAUSE menu (childScreen->Gameplay while
// screen stays Pause); a starter-opened browser returns to Starter. Both are screen-derived —
// no back-path code was added, this pins that the origins stay distinct.
bool pauseOpenedBrowserBackReturnsToPause() {
  iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window;
  window.gameplay.gameplayActive = true;
  bool closeRequested = false;
  iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::None;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppOptions options;
  iggy3d::ProductSaveBridgeResult saves = compatibleSaveBridge();
  iggy3d::FrontendSettings settings;

  (void)iggy3d::applyProductSystemPauseMenuAction(
      iggy3d::InputAction::SystemPause, {frontend, window, closeRequested});
  frontend.selectedAction = iggy3d::FrontendAction::LoadSave;
  (void)iggy3d::applyProductPauseMenuAction(
      iggy3d::InputAction::MenuConfirm,
      {frontend, options, saves, settingsTab, activeSession, window,
       closeRequested, settings});
  // Back out of the pause-opened browser.
  (void)iggy3d::applyProductLoadSaveMenuAction(
      iggy3d::InputAction::MenuBack,
      {frontend, options, saves, activeSession, window});
  const bool backToPause =
      frontend.screen == iggy3d::FrontendScreen::Pause &&
      frontend.childScreen == iggy3d::FrontendScreen::Gameplay;

  // Contrast: a starter-opened browser backs out to Starter.
  iggy3d::FrontendState starter = starterFrontend();
  starter.childScreen = iggy3d::FrontendScreen::LoadSave;
  starter.saveBrowserMode = iggy3d::FrontendSaveBrowserMode::Load;
  (void)iggy3d::applyProductLoadSaveMenuAction(
      iggy3d::InputAction::MenuBack,
      {starter, options, saves, activeSession, window});
  const bool backToStarter =
      starter.screen == iggy3d::FrontendScreen::Starter &&
      starter.childScreen == iggy3d::FrontendScreen::Gameplay;

  return expect(backToPause, "pause-opened browser back returns to the pause menu") &&
         expect(backToStarter, "starter-opened browser back returns to starter");
}

// sd3 HARD-STOP verify: load-from-pause goes through the SAME shared launch path as starter and
// replaces the active session — no pause-specific teardown. Uses a real durable save on disk.
bool loadFromPauseReplacesTheActiveSession() {
  namespace fs = std::filesystem;
  std::error_code ec;
  const fs::path saveRoot = fs::temp_directory_path() / "iggy3d_sd3_load_from_pause";
  fs::remove_all(saveRoot, ec);
  fs::create_directories(saveRoot, ec);

  iggy3d::ProductAppOptions options;
  options.saveRoot = saveRoot;
  iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window;
  window.gameplay.gameplayActive = true;
  bool closeRequested = false;
  iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::None;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::WorldSetupDraft draft;
  iggy3d::FrontendSettings settings;

  // Establish a durable save + an in-game session via new-world (writes to disk).
  iggy3d::launchProductNewWorld(options, draft, frontend, activeSession, window);
  const bool hadSession = activeSession.has_value();
  const iggy3d::ProductWorldTemplate world =
      iggy3d::productWorldTemplateFromOptions(options);
  iggy3d::ProductSaveBridgeResult saves =
      iggy3d::scanProductSaves(saveRoot, world.packageId, world.scenarioId);
  const bool haveCompatibleSave = saves.slots.compatibleCount > 0U;

  // Pause, open Load from pause, then confirm the load.
  (void)iggy3d::applyProductSystemPauseMenuAction(
      iggy3d::InputAction::SystemPause, {frontend, window, closeRequested});
  frontend.selectedAction = iggy3d::FrontendAction::LoadSave;
  (void)iggy3d::applyProductPauseMenuAction(
      iggy3d::InputAction::MenuConfirm,
      {frontend, options, saves, settingsTab, activeSession, window,
       closeRequested, settings});
  window.frontendShell.launchStatus = "unset";
  (void)iggy3d::applyProductLoadSaveMenuAction(
      iggy3d::InputAction::MenuConfirm,
      {frontend, options, saves, activeSession, window});
  const bool replaced =
      activeSession.has_value() && window.frontendShell.launchStatus == "product_save_loaded";

  fs::remove_all(saveRoot, ec);
  return expect(hadSession, "new-world established an active session") &&
         expect(haveCompatibleSave, "durable save is scannable/compatible") &&
         expect(replaced,
                "load-from-pause replaced the session via the shared launch path");
}

bool childPanelHitTestsExposeMenuActions() {
  iggy3d::FrontendState frontend = starterFrontend();
  frontend.childScreen = iggy3d::FrontendScreen::NewWorld;
  const iggy3d::OpeningMenuHitTestResult createHit =
      hitAt(frontend, 452.0F, 508.0F);
  const iggy3d::OpeningMenuHitTestResult newWorldBackHit =
      hitAt(frontend, 850.0F, 508.0F);
  const iggy3d::OpeningMenuHitTestResult newWorldPreviousHit =
      hitAt(frontend, 452.0F, 556.0F);
  const iggy3d::OpeningMenuHitTestResult newWorldNextHit =
      hitAt(frontend, 570.0F, 556.0F);

  frontend.childScreen = iggy3d::FrontendScreen::LoadSave;
  const iggy3d::OpeningMenuHitTestResult slotHit =
      hitAt(frontend, 452.0F, 356.0F);
  const iggy3d::OpeningMenuHitTestResult loadHit =
      hitAt(frontend, 452.0F, 548.0F);
  const iggy3d::OpeningMenuHitTestResult deleteHit =
      hitAt(frontend, 690.0F, 548.0F);
  const iggy3d::OpeningMenuHitTestResult loadBackHit =
      hitAt(frontend, 1010.0F, 548.0F);

  frontend.childScreen = iggy3d::FrontendScreen::DeleteConfirm;
  const iggy3d::OpeningMenuHitTestResult confirmDeleteHit =
      hitAt(frontend, 452.0F, 508.0F);
  const iggy3d::OpeningMenuHitTestResult deleteBackHit =
      hitAt(frontend, 760.0F, 508.0F);
  frontend.childScreen = iggy3d::FrontendScreen::Settings;
  const iggy3d::OpeningMenuHitTestResult settingsBackHit =
      hitAt(frontend, 850.0F, 394.0F);
  frontend.screen = iggy3d::FrontendScreen::Settings;
  frontend.childScreen = iggy3d::FrontendScreen::Pause;
  const iggy3d::OpeningMenuHitTestResult pauseSettingsBackHit =
      hitAt(frontend, 850.0F, 394.0F);
  const iggy3d::OpeningMenuHitTestResult pauseSettingsTabHit =
      hitAt(frontend, 452.0F, 264.0F);
  frontend.screen = iggy3d::FrontendScreen::Starter;
  frontend.childScreen = iggy3d::FrontendScreen::StarterDevTools;
  const iggy3d::OpeningMenuHitTestResult devToolsBackHit =
      hitAt(frontend, 850.0F, 394.0F);

  return expect(createHit.hit, "new world create hit") &&
         expect(createHit.area == iggy3d::OpeningMenuHitArea::NewWorldCreate,
                "new world create area") &&
         expect(newWorldBackHit.hit, "new world back hit") &&
         expect(newWorldBackHit.area == iggy3d::OpeningMenuHitArea::NewWorldBack,
                "new world back area") &&
         expect(newWorldPreviousHit.hit, "new world previous dungeon hit") &&
         expect(newWorldPreviousHit.area ==
                    iggy3d::OpeningMenuHitArea::NewWorldPreviousDungeon,
                "new world previous dungeon area") &&
         expect(newWorldNextHit.hit, "new world next dungeon hit") &&
         expect(newWorldNextHit.area ==
                    iggy3d::OpeningMenuHitArea::NewWorldNextDungeon,
                "new world next dungeon area") &&
         expect(slotHit.hit, "load save slot hit") &&
         expect(slotHit.area == iggy3d::OpeningMenuHitArea::LoadSaveSlot,
                "load save slot area") &&
         expect(slotHit.saveSlotIndex == 1U, "load save slot index") &&
         expect(loadHit.hit, "load selected hit") &&
         expect(loadHit.area == iggy3d::OpeningMenuHitArea::LoadSaveLoad,
                "load selected area") &&
         expect(deleteHit.hit, "delete selected hit") &&
         expect(deleteHit.area == iggy3d::OpeningMenuHitArea::LoadSaveDelete,
                "delete selected area") &&
         expect(loadBackHit.hit, "load save back hit") &&
         expect(loadBackHit.area == iggy3d::OpeningMenuHitArea::LoadSaveBack,
                "load save back area") &&
         expect(confirmDeleteHit.hit, "confirm delete hit") &&
         expect(confirmDeleteHit.area ==
                    iggy3d::OpeningMenuHitArea::DeleteConfirmConfirm,
                "confirm delete area") &&
         expect(deleteBackHit.hit, "delete confirm back hit") &&
         expect(deleteBackHit.area ==
                    iggy3d::OpeningMenuHitArea::DeleteConfirmBack,
                "delete confirm back area") &&
         expect(settingsBackHit.hit, "settings back hit") &&
         expect(settingsBackHit.area == iggy3d::OpeningMenuHitArea::SettingsBack,
                "settings back area") &&
         expect(pauseSettingsBackHit.hit, "pause settings back hit") &&
         expect(pauseSettingsBackHit.area ==
                    iggy3d::OpeningMenuHitArea::SettingsBack,
                "pause settings back area") &&
         expect(pauseSettingsTabHit.hit, "pause settings tab hit") &&
         expect(pauseSettingsTabHit.area ==
                    iggy3d::OpeningMenuHitArea::SettingsTab,
                "pause settings tab area") &&
         expect(pauseSettingsTabHit.settingsTab ==
                    iggy3d::FrontendSettingsTab::Controls,
                "pause settings tab target") &&
         expect(devToolsBackHit.hit, "dev tools back hit") &&
         expect(devToolsBackHit.area == iggy3d::OpeningMenuHitArea::DevToolsBack,
                "dev tools back area");
}

bool openingMenuMouseDispatchRoutesStarterAndSettingsHits() {
  MouseDispatchHarness starter;
  iggy3d::OpeningMenuHitTestResult starterHit =
      hitArea(iggy3d::OpeningMenuHitArea::StarterAction);
  starterHit.action = iggy3d::FrontendAction::Settings;
  iggy3d::dispatchProductOpeningMenuMouseHit(starterHit,
                                             starter.click,
                                             starter.actionState,
                                             starter.context());
  const bool starterOk =
      expect(starter.frontend.childScreen == iggy3d::FrontendScreen::Settings,
             "mouse starter action opens settings") &&
      expect(starter.settingsTab == iggy3d::FrontendSettingsTab::Input,
             "mouse starter settings tab") &&
      expect(starter.window.inputDevice.lastInputAccepted,
             "mouse starter action accepted");

  MouseDispatchHarness settings;
  settings.frontend = gameplayFrontend();
  settings.frontend.screen = iggy3d::FrontendScreen::Settings;
  settings.frontend.childScreen = iggy3d::FrontendScreen::Pause;
  settings.window.gameplay.gameplayActive = true;
  settings.settingsTab = iggy3d::FrontendSettingsTab::Input;
  iggy3d::OpeningMenuHitTestResult tabHit =
      hitArea(iggy3d::OpeningMenuHitArea::SettingsTab);
  tabHit.settingsTab = iggy3d::FrontendSettingsTab::Controls;
  iggy3d::dispatchProductOpeningMenuMouseHit(tabHit,
                                             settings.click,
                                             settings.actionState,
                                             settings.context());
  const bool tabOk =
      expect(settings.settingsTab == iggy3d::FrontendSettingsTab::Controls,
             "mouse settings tab selected") &&
      expect(settings.frontend.status == "settings_tab_selected",
             "mouse settings tab status");

  iggy3d::dispatchProductOpeningMenuMouseHit(
      hitArea(iggy3d::OpeningMenuHitArea::SettingsBack),
      settings.click,
      settings.actionState,
      settings.context());
  return starterOk && tabOk &&
         expect(settings.frontend.screen == iggy3d::FrontendScreen::Pause,
                "mouse settings back returns pause") &&
         expect(liveSurface(settings.frontend, settings.window).inputOwner == iggy3d::MenuOwner::Pause,
                "mouse settings back owner pause");
}

bool openingMenuMouseDispatchRoutesNewWorldNavigationRows() {
  MouseDispatchHarness harness;
  harness.frontend.childScreen = iggy3d::FrontendScreen::NewWorld;
  iggy3d::recordWorldSetupDraftState(harness.draft, harness.window);
  const std::string initialDungeonId = harness.draft.asciiRoomId;

  iggy3d::dispatchProductOpeningMenuMouseHit(
      hitArea(iggy3d::OpeningMenuHitArea::NewWorldNextDungeon),
      harness.click,
      harness.actionState,
      harness.context());
  const bool nextOk =
      expect(harness.draft.asciiRoomId != initialDungeonId,
             "mouse next dungeon changes draft") &&
      expect(harness.frontend.status == "new_world_dungeon_selection_changed",
             "mouse next dungeon status");
  const std::string nextDungeonId = harness.draft.asciiRoomId;

  iggy3d::dispatchProductOpeningMenuMouseHit(
      hitArea(iggy3d::OpeningMenuHitArea::NewWorldPreviousDungeon),
      harness.click,
      harness.actionState,
      harness.context());
  const bool previousOk =
      expect(harness.draft.asciiRoomId != nextDungeonId,
             "mouse previous dungeon changes draft") &&
      expect(harness.window.creativeAuthoring.worldSetup.status == "world_setup_dungeon_selected",
             "mouse previous dungeon window status");

  iggy3d::dispatchProductOpeningMenuMouseHit(
      hitArea(iggy3d::OpeningMenuHitArea::NewWorldBack),
      harness.click,
      harness.actionState,
      harness.context());
  return nextOk && previousOk &&
         expect(harness.frontend.childScreen == iggy3d::FrontendScreen::Gameplay,
                "mouse new world back closes child");
}

bool openingMenuMouseDispatchRoutesLoadSaveRows() {
  MouseDispatchHarness slot;
  slot.frontend.childScreen = iggy3d::FrontendScreen::LoadSave;
  iggy3d::OpeningMenuHitTestResult slotHit =
      hitArea(iggy3d::OpeningMenuHitArea::LoadSaveSlot);
  slotHit.saveSlotIndex = 0U;
  iggy3d::dispatchProductOpeningMenuMouseHit(slotHit,
                                             slot.click,
                                             slot.actionState,
                                             slot.context());
  const bool slotOk =
      expect(slot.window.saveSession.selectedProductSave.id == "save_unit",
             "mouse load slot selects save") &&
      expect(slot.frontend.status == "load_save_selection_changed",
             "mouse load slot status");

  MouseDispatchHarness deleteSave;
  deleteSave.frontend.childScreen = iggy3d::FrontendScreen::LoadSave;
  iggy3d::OpeningMenuHitTestResult deleteSlotHit =
      hitArea(iggy3d::OpeningMenuHitArea::LoadSaveSlot);
  deleteSlotHit.saveSlotIndex = 0U;
  iggy3d::dispatchProductOpeningMenuMouseHit(deleteSlotHit,
                                             deleteSave.click,
                                             deleteSave.actionState,
                                             deleteSave.context());
  iggy3d::dispatchProductOpeningMenuMouseHit(
      hitArea(iggy3d::OpeningMenuHitArea::LoadSaveDelete),
      deleteSave.click,
      deleteSave.actionState,
      deleteSave.context());
  const bool deleteOk =
      expect(deleteSave.frontend.selectedAction == iggy3d::FrontendAction::Delete,
             "mouse load delete selected action") &&
      expect(deleteSave.frontend.childScreen ==
                 iggy3d::FrontendScreen::DeleteConfirm,
             "mouse load delete opens confirm") &&
      expect(deleteSave.window.saveSession.saveDelete.confirmationOpen,
             "mouse load delete confirmation open");

  MouseDispatchHarness back;
  back.frontend.childScreen = iggy3d::FrontendScreen::LoadSave;
  iggy3d::dispatchProductOpeningMenuMouseHit(
      hitArea(iggy3d::OpeningMenuHitArea::LoadSaveBack),
      back.click,
      back.actionState,
      back.context());
  return slotOk && deleteOk &&
         expect(back.frontend.childScreen == iggy3d::FrontendScreen::Gameplay,
                "mouse load back closes child") &&
         expect(back.frontend.status == "load_save_closed",
                "mouse load back status");
}

bool openingMenuMouseDispatchReportsUnhandledHitArea() {
  MouseDispatchHarness harness;
  iggy3d::dispatchProductOpeningMenuMouseHit(
      hitArea(iggy3d::OpeningMenuHitArea::None),
      harness.click,
      harness.actionState,
      harness.context());
  return expect(harness.frontend.status == "opening_menu_hit_area_unhandled",
                "unhandled mouse hit status");
}

bool menuClickNormalizationScalesWindowCoordinates() {
  const iggy3d::MouseClick scaled =
      iggy3d::normalizeProductWindowMenuClick(clickAt(226.0F, 254.0F),
                                              640U,
                                              360U);
  const iggy3d::MouseClick unchanged =
      iggy3d::normalizeProductWindowMenuClick(clickAt(452.0F, 508.0F),
                                              0U,
                                              360U);
  iggy3d::FrontendState frontend = starterFrontend();
  frontend.childScreen = iggy3d::FrontendScreen::NewWorld;
  const iggy3d::OpeningMenuHitTestResult createHit =
      hitAt(frontend, scaled.x, scaled.y);

  return expect(scaled.clicked, "scaled click remains clicked") &&
         expect(scaled.x == 452.0F, "scaled click x") &&
         expect(scaled.y == 508.0F, "scaled click y") &&
         expect(createHit.hit, "scaled click hits create") &&
         expect(createHit.area == iggy3d::OpeningMenuHitArea::NewWorldCreate,
                "scaled click create area") &&
         expect(unchanged.x == 452.0F, "zero window leaves x") &&
         expect(unchanged.y == 508.0F, "zero window leaves y");
}

bool devToggleOpensAndClosesDevToolsSurfaces() {
  {
    iggy3d::FrontendState frontend = starterFrontend();
    iggy3d::ProductAppWindowState window;
    bool closeRequested = false;
    const iggy3d::ProductMenuActionResult opened =
        iggy3d::applyProductSystemPauseMenuAction(
            iggy3d::InputAction::DevToggle,
            {frontend, window, closeRequested});
    const bool openedChild =
        frontend.childScreen == iggy3d::FrontendScreen::StarterDevTools;
    const bool openedFlag = iggy3d::frontendDevToolsOpen(frontend);
    const iggy3d::MenuOwner openedOwner = liveSurface(frontend, window).inputOwner;
    const iggy3d::ProductMenuActionResult closed =
        iggy3d::applyProductSystemPauseMenuAction(
            iggy3d::InputAction::DevToggle,
            {frontend, window, closeRequested});
    const bool starterOk =
        expect(opened.handled, "starter dev toggle handled") &&
        expect(opened.accepted, "starter dev toggle accepted") &&
        expect(openedChild, "starter dev toggle opens child") &&
        expect(openedFlag, "starter dev tools open") &&
        expect(openedOwner == iggy3d::MenuOwner::DevTools,
               "starter dev toggle owner") &&
        expect(frontend.childScreen == iggy3d::FrontendScreen::Gameplay,
               "starter dev toggle closes child") &&
        expect(closed.handled, "starter dev close handled") &&
        expect(closed.accepted, "starter dev close accepted") &&
        expect(!iggy3d::frontendDevToolsOpen(frontend),
               "starter dev tools closed") &&
        expect(!closeRequested, "dev toggle does not close window");
    if (!starterOk) {
      return false;
    }
  }

  iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window;
  window.gameplay.gameplayActive = true;
  bool closeRequested = false;
  const iggy3d::ProductMenuActionResult opened =
      iggy3d::applyProductSystemPauseMenuAction(
          iggy3d::InputAction::DevToggle,
          {frontend, window, closeRequested});
  const bool openedOk =
      expect(opened.handled, "gameplay dev toggle handled") &&
      expect(opened.accepted, "gameplay dev toggle accepted") &&
      expect(frontend.screen == iggy3d::FrontendScreen::DevOverlay,
             "gameplay dev toggle opens overlay") &&
      expect(iggy3d::frontendDevToolsOpen(frontend),
             "gameplay dev tools open") &&
      expect(liveSurface(frontend, window).inputOwner == iggy3d::MenuOwner::DevTools,
             "gameplay dev toggle owner");
  const iggy3d::ProductMenuActionResult closed =
      iggy3d::applyProductSystemPauseMenuAction(
          iggy3d::InputAction::DevToggle,
          {frontend, window, closeRequested});
  return openedOk &&
         expect(closed.handled, "gameplay dev close handled") &&
         expect(closed.accepted, "gameplay dev close accepted") &&
         expect(frontend.screen == iggy3d::FrontendScreen::Gameplay,
                "gameplay dev toggle closes overlay") &&
         expect(liveSurface(frontend, window).inputOwner == iggy3d::MenuOwner::Gameplay,
                "gameplay dev close owner") &&
         expect(!closeRequested, "gameplay dev toggle no close");
}

bool debugOverlayActionTogglesRuntimeOverlaySetting() {
  iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window;
  window.gameplay.gameplayActive = true;
  iggy3d::FrontendSettings settings;
  bool closeRequested = false;

  const iggy3d::ProductMenuActionResult enabled =
      iggy3d::applyProductSystemPauseMenuAction(
          iggy3d::InputAction::DevDebugOverlay,
          {frontend, window, closeRequested, &settings});
  const bool enabledOk =
      expect(enabled.handled, "debug overlay enable handled") &&
      expect(enabled.accepted, "debug overlay enable accepted") &&
      expect(settings.debugOverlayEnabled, "debug overlay setting enabled") &&
      expect(frontend.status == "debug_overlay_enabled",
             "debug overlay enabled status") &&
      expect(frontend.screen == iggy3d::FrontendScreen::Gameplay,
             "debug overlay does not open dev screen") &&
      expect(!iggy3d::frontendDevToolsOpen(frontend),
             "debug overlay does not open dev tools") &&
      expect(!closeRequested, "debug overlay does not close window");

  const iggy3d::ProductMenuActionResult disabled =
      iggy3d::applyProductSystemPauseMenuAction(
          iggy3d::InputAction::DevDebugOverlay,
          {frontend, window, closeRequested, &settings});
  const bool disabledOk =
      expect(disabled.handled, "debug overlay disable handled") &&
      expect(disabled.accepted, "debug overlay disable accepted") &&
      expect(!settings.debugOverlayEnabled, "debug overlay setting disabled") &&
      expect(frontend.status == "debug_overlay_disabled",
             "debug overlay disabled status") &&
      expect(frontend.screen == iggy3d::FrontendScreen::Gameplay,
             "debug overlay disable keeps gameplay screen");

  iggy3d::FrontendSettings missingSettings;
  missingSettings.debugOverlayEnabled = false;
  const iggy3d::ProductMenuActionResult missing =
      iggy3d::applyProductSystemPauseMenuAction(
          iggy3d::InputAction::DevDebugOverlay,
          {frontend, window, closeRequested});

  return enabledOk && disabledOk &&
         expect(missing.handled, "debug overlay missing settings handled") &&
         expect(!missing.accepted, "debug overlay missing settings rejected") &&
         expect(!missingSettings.debugOverlayEnabled,
                "debug overlay missing settings does not mutate unrelated state");
}

bool debugOverlayActionIgnoresFrontendBlockedSurfaces() {
  {
    iggy3d::FrontendState frontend = starterFrontend();
    iggy3d::ProductAppWindowState window;
    iggy3d::FrontendSettings settings;
    settings.debugOverlayEnabled = false;
    bool closeRequested = false;

    const iggy3d::ProductMenuActionResult blocked =
        iggy3d::applyProductSystemPauseMenuAction(
            iggy3d::InputAction::DevDebugOverlay,
            {frontend, window, closeRequested, &settings});
    const bool starterOk =
        expect(blocked.handled, "starter F3 handled") &&
        expect(!blocked.accepted, "starter F3 rejected") &&
        expect(!settings.debugOverlayEnabled, "starter F3 does not enable debug") &&
        expect(frontend.status == "debug_overlay_gameplay_inactive",
               "starter F3 inactive status") &&
        expect(!closeRequested, "starter F3 does not close window");
    if (!starterOk) {
      return false;
    }
  }

  {
    iggy3d::FrontendState frontend = starterFrontend();
    frontend.childScreen = iggy3d::FrontendScreen::NewWorld;
    iggy3d::ProductAppWindowState window;
    window.gameplay.gameplayActive = true;
    iggy3d::FrontendSettings settings;
    settings.debugOverlayEnabled = false;
    bool closeRequested = false;

    const iggy3d::ProductMenuActionResult blocked =
        iggy3d::applyProductSystemPauseMenuAction(
            iggy3d::InputAction::DevDebugOverlay,
            {frontend, window, closeRequested, &settings});
    const bool newWorldOk =
        expect(blocked.handled, "new-world F3 handled") &&
        expect(!blocked.accepted, "new-world F3 rejected") &&
        expect(!settings.debugOverlayEnabled,
               "new-world F3 does not enable debug") &&
        expect(frontend.status == "debug_overlay_gameplay_inactive",
               "new-world F3 inactive status");
    if (!newWorldOk) {
      return false;
    }
  }

  {
    iggy3d::FrontendState frontend = gameplayFrontend();
    frontend.screen = iggy3d::FrontendScreen::Settings;
    frontend.childScreen = iggy3d::FrontendScreen::Pause;
    iggy3d::ProductAppWindowState window;
    window.gameplay.gameplayActive = true;
    iggy3d::FrontendSettings settings;
    settings.debugOverlayEnabled = false;
    bool closeRequested = false;

    const iggy3d::ProductMenuActionResult blocked =
        iggy3d::applyProductSystemPauseMenuAction(
            iggy3d::InputAction::DevDebugOverlay,
            {frontend, window, closeRequested, &settings});
    const bool pauseSettingsOk =
        expect(blocked.handled, "pause-settings F3 handled") &&
        expect(!blocked.accepted, "pause-settings F3 rejected") &&
        expect(!settings.debugOverlayEnabled,
               "pause-settings F3 does not enable debug") &&
        expect(frontend.status == "debug_overlay_gameplay_inactive",
               "pause-settings F3 inactive status");
    if (!pauseSettingsOk) {
      return false;
    }
  }

  iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window;
  window.gameplay.gameplayActive = true;
  bool closeRequested = false;
  const iggy3d::ProductMenuActionResult opened =
      iggy3d::applyProductSystemPauseMenuAction(
          iggy3d::InputAction::DevToggle,
          {frontend, window, closeRequested});
  iggy3d::FrontendSettings settings;
  settings.debugOverlayEnabled = false;
  const iggy3d::ProductMenuActionResult blocked =
      iggy3d::applyProductSystemPauseMenuAction(
          iggy3d::InputAction::DevDebugOverlay,
          {frontend, window, closeRequested, &settings});
  const bool blockedOk =
      expect(opened.handled, "dev-tools F1 open handled") &&
      expect(opened.accepted, "dev-tools F1 open accepted") &&
      expect(blocked.handled, "dev-tools F3 handled") &&
      expect(!blocked.accepted, "dev-tools F3 rejected") &&
      expect(!settings.debugOverlayEnabled,
             "dev-tools F3 does not enable debug") &&
      expect(frontend.status == "debug_overlay_gameplay_inactive",
             "dev-tools F3 inactive status");
  const iggy3d::ProductMenuActionResult closed =
      iggy3d::applyProductSystemPauseMenuAction(
          iggy3d::InputAction::DevToggle,
          {frontend, window, closeRequested, &settings});

  return blockedOk &&
         expect(closed.handled, "dev-tools F1 close handled") &&
         expect(closed.accepted, "dev-tools F1 close accepted");
}

bool collisionOverlayActionTogglesDistinctWindowState() {
  iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window;
  window.gameplay.gameplayActive = true;
  iggy3d::FrontendSettings settings;
  bool closeRequested = false;

  const iggy3d::ProductMenuActionResult enabled =
      iggy3d::applyProductSystemPauseMenuAction(
          iggy3d::InputAction::DevCollisionOverlay,
          {frontend, window, closeRequested, &settings});
  const bool enabledOk =
      expect(enabled.handled, "collision overlay enable handled") &&
      expect(enabled.accepted, "collision overlay enable accepted") &&
      expect(window.debugHud.devCollisionOverlay.visible,
             "collision overlay window flag enabled") &&
      expect(window.debugHud.devCollisionOverlay.status ==
                 "dev_collision_overlay_enabled",
             "collision overlay enabled status") &&
      expect(!settings.debugOverlayEnabled,
             "collision overlay does not toggle debug overlay setting") &&
      expect(frontend.screen == iggy3d::FrontendScreen::Gameplay,
             "collision overlay keeps gameplay screen") &&
      expect(!iggy3d::frontendDevToolsOpen(frontend),
             "collision overlay does not open dev tools") &&
      expect(!closeRequested, "collision overlay does not close window");

  const iggy3d::ProductMenuActionResult disabled =
      iggy3d::applyProductSystemPauseMenuAction(
          iggy3d::InputAction::DevCollisionOverlay,
          {frontend, window, closeRequested, &settings});
  const bool disabledOk =
      expect(disabled.handled, "collision overlay disable handled") &&
      expect(disabled.accepted, "collision overlay disable accepted") &&
      expect(!window.debugHud.devCollisionOverlay.visible,
             "collision overlay window flag hidden") &&
      expect(window.debugHud.devCollisionOverlay.status ==
                 "dev_collision_overlay_hidden",
             "collision overlay hidden status") &&
      expect(!settings.debugOverlayEnabled,
             "collision overlay disabled leaves debug overlay unchanged");

  const iggy3d::ProductMenuActionResult devTools =
      iggy3d::applyProductSystemPauseMenuAction(
          iggy3d::InputAction::DevToggle,
          {frontend, window, closeRequested, &settings});
  return enabledOk && disabledOk &&
         expect(devTools.handled, "dev toggle after collision handled") &&
         expect(devTools.accepted, "dev toggle after collision accepted") &&
         expect(iggy3d::frontendDevToolsOpen(frontend),
                "dev toggle still opens dev tools") &&
         expect(!window.debugHud.devCollisionOverlay.visible,
                "dev toggle does not toggle collision overlay");
}

bool sdlFunctionKeyEventsMapToSystemActions() {
  iggy3d::SdlWindowEventState none;
  iggy3d::SdlWindowEventState f1;
  f1.f1Pressed = true;
  iggy3d::SdlWindowEventState f2;
  f2.f2Pressed = true;
  iggy3d::SdlWindowEventState f3;
  f3.f3Pressed = true;
  iggy3d::SdlWindowEventState f4;
  f4.f4Pressed = true;
  iggy3d::SdlWindowEventState m;
  m.mPressed = true;
  iggy3d::SdlWindowEventState f1f3;
  f1f3.f1Pressed = true;
  f1f3.f3Pressed = true;
  iggy3d::SdlWindowEventState f3f4m;
  f3f4m.f3Pressed = true;
  f3f4m.f4Pressed = true;
  f3f4m.mPressed = true;
  iggy3d::SdlWindowEventState f4f1m;
  f4f1m.f4Pressed = true;
  f4f1m.f1Pressed = true;
  f4f1m.mPressed = true;
  iggy3d::SdlWindowEventState f1f2;
  f1f2.f1Pressed = true;
  f1f2.f2Pressed = true;
  iggy3d::SdlWindowEventState f2m;
  f2m.f2Pressed = true;
  f2m.mPressed = true;

  return expect(iggy3d::productWindowFunctionKeyAction(none) ==
                    iggy3d::InputAction::None,
                "no function key maps to no action") &&
         expect(iggy3d::productWindowFunctionKeyAction(f1) ==
                    iggy3d::InputAction::DevToggle,
                "F1 event maps to dev toggle") &&
         expect(iggy3d::productWindowFunctionKeyAction(f2) ==
                    iggy3d::InputAction::DevCollisionOverlay,
                "F2 event maps to collision overlay") &&
         expect(iggy3d::productWindowFunctionKeyAction(f3) ==
                    iggy3d::InputAction::DevDebugOverlay,
                "F3 event maps to debug overlay") &&
         expect(iggy3d::productWindowFunctionKeyAction(f4) ==
                    iggy3d::InputAction::MovementTuningToggle,
                "F4 event maps to movement tuning") &&
         expect(iggy3d::productWindowFunctionKeyAction(m) ==
                    iggy3d::InputAction::MapMakerToggle,
                "M event maps to map maker toggle") &&
         expect(iggy3d::productWindowFunctionKeyAction(f1f3) ==
                    iggy3d::InputAction::DevDebugOverlay,
                "F3 overlay event wins over dev panel toggle") &&
         expect(iggy3d::productWindowFunctionKeyAction(f3f4m) ==
                    iggy3d::InputAction::DevDebugOverlay,
                "F3 wins over F4 and M in function key priority") &&
         expect(iggy3d::productWindowFunctionKeyAction(f4f1m) ==
                    iggy3d::InputAction::MovementTuningToggle,
                "F4 wins over F1 and M in function key priority") &&
         expect(iggy3d::productWindowFunctionKeyAction(f1f2) ==
                    iggy3d::InputAction::DevToggle,
                "F1 wins over F2 in one-frame function key priority") &&
         expect(iggy3d::productWindowFunctionKeyAction(f2m) ==
                    iggy3d::InputAction::DevCollisionOverlay,
                "F2 wins over M in function key priority");
}

bool sdlFunctionKeyEventsMarkKeyboardStateConsumed() {
  iggy3d::KeyboardInputState keyboard;
  iggy3d::SdlWindowEventState none;
  iggy3d::recordProductWindowFunctionKeyKeyboardState(keyboard, none);
  const bool noneOk =
      expect(!keyboard.devToggleWasDown, "no F-key leaves dev toggle unconsumed") &&
      expect(!keyboard.devCollisionOverlayWasDown,
             "no F-key leaves collision overlay unconsumed") &&
      expect(!keyboard.debugOverlayWasDown,
             "no F-key leaves debug overlay unconsumed") &&
      expect(!keyboard.movementTuningToggleWasDown,
             "no F-key leaves movement tuning unconsumed") &&
      expect(!keyboard.mapMakerToggleWasDown,
             "no M key leaves map maker unconsumed");

  iggy3d::SdlWindowEventState f2;
  f2.f2Pressed = true;
  iggy3d::recordProductWindowFunctionKeyKeyboardState(keyboard, f2);
  const bool f2Ok =
      expect(!keyboard.devToggleWasDown,
             "F2 event does not consume dev toggle state") &&
      expect(keyboard.devCollisionOverlayWasDown,
             "F2 event consumes collision overlay state") &&
      expect(!keyboard.debugOverlayWasDown,
             "F2 event does not consume debug overlay state") &&
      expect(!keyboard.movementTuningToggleWasDown,
             "F2 event does not consume movement tuning state") &&
      expect(!keyboard.mapMakerToggleWasDown,
             "F2 event does not consume map maker state");

  keyboard = {};
  iggy3d::SdlWindowEventState f3;
  f3.f3Pressed = true;
  iggy3d::recordProductWindowFunctionKeyKeyboardState(keyboard, f3);
  const bool f3Ok =
      expect(!keyboard.devToggleWasDown,
             "F3 event does not consume dev toggle state") &&
      expect(!keyboard.devCollisionOverlayWasDown,
             "F3 event does not consume collision overlay state") &&
      expect(keyboard.debugOverlayWasDown,
             "F3 event consumes debug overlay state") &&
      expect(!keyboard.movementTuningToggleWasDown,
             "F3 event does not consume movement tuning state") &&
      expect(!keyboard.mapMakerToggleWasDown,
             "F3 event does not consume map maker state");

  keyboard = {};
  iggy3d::SdlWindowEventState f4;
  f4.f4Pressed = true;
  iggy3d::recordProductWindowFunctionKeyKeyboardState(keyboard, f4);
  const bool f4Ok =
      expect(!keyboard.devToggleWasDown,
             "F4 event does not consume dev toggle state") &&
      expect(!keyboard.devCollisionOverlayWasDown,
             "F4 event does not consume collision overlay state") &&
      expect(!keyboard.debugOverlayWasDown,
             "F4 event does not consume debug overlay state") &&
      expect(keyboard.movementTuningToggleWasDown,
             "F4 event consumes movement tuning state") &&
      expect(!keyboard.mapMakerToggleWasDown,
             "F4 event does not consume map maker state");

  keyboard = {};
  iggy3d::SdlWindowEventState m;
  m.mPressed = true;
  iggy3d::recordProductWindowFunctionKeyKeyboardState(keyboard, m);
  const bool mOk =
      expect(!keyboard.devToggleWasDown,
             "M event does not consume dev toggle state") &&
      expect(!keyboard.devCollisionOverlayWasDown,
             "M event does not consume collision overlay state") &&
      expect(!keyboard.debugOverlayWasDown,
             "M event does not consume debug overlay state") &&
      expect(!keyboard.movementTuningToggleWasDown,
             "M event does not consume movement tuning state") &&
      expect(keyboard.mapMakerToggleWasDown,
             "M event consumes map maker state");

  keyboard = {};
  iggy3d::SdlWindowEventState f1f2;
  f1f2.f1Pressed = true;
  f1f2.f2Pressed = true;
  iggy3d::recordProductWindowFunctionKeyKeyboardState(keyboard, f1f2);
  const bool f1f2Ok =
      expect(keyboard.devToggleWasDown,
             "F1+F2 event consumes dev toggle state") &&
      expect(keyboard.devCollisionOverlayWasDown,
             "F1+F2 event consumes collision overlay state") &&
      expect(!keyboard.debugOverlayWasDown,
             "F1+F2 event does not consume debug overlay state");

  return noneOk && f2Ok && f3Ok && f4Ok && mOk && f1f2Ok;
}

bool topLevelToggleFunnelPreservesPolicies() {
  iggy3d::FrontendSettings settings;
  bool closeRequested = false;

  iggy3d::FrontendState gameplay = gameplayFrontend();
  iggy3d::ProductAppWindowState gameplayWindow;
  gameplayWindow.gameplay.gameplayActive = true;
  const iggy3d::ProductWindowTopLevelToggleResult debug =
      iggy3d::dispatchProductWindowTopLevelToggleAction(
          gameplay,
          gameplayWindow,
          iggy3d::InputAction::DevDebugOverlay,
          &settings,
          &closeRequested);
  const bool debugOk =
      expect(debug.handled, "top-level F3 handled") &&
      expect(debug.accepted, "top-level F3 accepted in gameplay") &&
      expect(debug.action == iggy3d::InputAction::DevDebugOverlay,
             "top-level F3 records action") &&
      expect(settings.debugOverlayEnabled, "top-level F3 toggles debug setting");

  iggy3d::FrontendState starter = starterFrontend();
  iggy3d::ProductAppWindowState starterWindow;
  settings.debugOverlayEnabled = false;
  const iggy3d::ProductWindowTopLevelToggleResult blockedDebug =
      iggy3d::dispatchProductWindowTopLevelToggleAction(
          starter,
          starterWindow,
          iggy3d::InputAction::DevDebugOverlay,
          &settings,
          &closeRequested);
  const bool blockedDebugOk =
      expect(blockedDebug.handled, "top-level starter F3 handled") &&
      expect(!blockedDebug.accepted, "top-level starter F3 rejected") &&
      expect(!settings.debugOverlayEnabled,
             "top-level starter F3 leaves debug setting unchanged");

  iggy3d::FrontendState tuningFrontend = gameplayFrontend();
  iggy3d::ProductAppWindowState tuningWindow;
  tuningWindow.gameplay.gameplayActive = true;
  const iggy3d::ProductWindowTopLevelToggleResult tuning =
      iggy3d::dispatchProductWindowTopLevelToggleAction(
          tuningFrontend,
          tuningWindow,
          iggy3d::InputAction::MovementTuningToggle,
          &settings,
          &closeRequested);
  const bool tuningOk =
      expect(tuning.handled, "top-level F4 handled") &&
      expect(tuning.accepted, "top-level F4 accepted in gameplay") &&
      expect(tuningWindow.gameplay.gameplayMovement.tuningVisible,
             "top-level F4 shows movement tuning");

  iggy3d::FrontendState pause = gameplayFrontend();
  pause.screen = iggy3d::FrontendScreen::Pause;
  pause.childScreen = iggy3d::FrontendScreen::Gameplay;
  iggy3d::ProductAppWindowState pauseWindow;
  pauseWindow.gameplay.gameplayActive = true;
  pauseWindow.gameplay.gameplayMovement.tuningVisible = true;
  const iggy3d::ProductWindowTopLevelToggleResult blockedTuning =
      iggy3d::dispatchProductWindowTopLevelToggleAction(
          pause,
          pauseWindow,
          iggy3d::InputAction::MovementTuningToggle,
          &settings,
          &closeRequested);
  const bool blockedTuningOk =
      expect(blockedTuning.handled, "top-level pause F4 handled") &&
      expect(!blockedTuning.accepted, "top-level pause F4 rejected") &&
      expect(!pauseWindow.gameplay.gameplayMovement.tuningVisible,
             "top-level pause F4 clears stale tuning");

  iggy3d::FrontendState devTools = starterFrontend();
  iggy3d::ProductAppWindowState devWindow;
  const iggy3d::ProductWindowTopLevelToggleResult dev =
      iggy3d::dispatchProductWindowTopLevelToggleAction(
          devTools,
          devWindow,
          iggy3d::InputAction::DevToggle,
          &settings,
          &closeRequested);
  const bool devOk =
      expect(dev.handled, "top-level F1 handled") &&
      expect(dev.accepted, "top-level F1 accepted") &&
      expect(iggy3d::frontendDevToolsOpen(devTools),
             "top-level F1 opens dev tools");

  iggy3d::FrontendState collision = gameplayFrontend();
  iggy3d::ProductAppWindowState collisionWindow;
  collisionWindow.gameplay.gameplayActive = true;
  settings.debugOverlayEnabled = false;
  const iggy3d::ProductWindowTopLevelToggleResult f2 =
      iggy3d::dispatchProductWindowTopLevelToggleAction(
          collision,
          collisionWindow,
          iggy3d::InputAction::DevCollisionOverlay,
          &settings,
          &closeRequested);
  const bool f2Ok =
      expect(f2.handled, "top-level F2 handled") &&
      expect(f2.accepted, "top-level F2 accepted") &&
      expect(collisionWindow.debugHud.devCollisionOverlay.visible,
             "top-level F2 toggles collision overlay") &&
      expect(!settings.debugOverlayEnabled,
             "top-level F2 does not mutate debug setting");

  iggy3d::FrontendState mapMaker = gameplayFrontend();
  iggy3d::ProductAppWindowState mapMakerWindow;
  mapMakerWindow.gameplay.gameplayActive = true;
  const iggy3d::ProductWindowTopLevelToggleResult m =
      iggy3d::dispatchProductWindowTopLevelToggleAction(
          mapMaker,
          mapMakerWindow,
          iggy3d::InputAction::MapMakerToggle,
          &settings,
          &closeRequested);
  const bool mOk =
      expect(m.handled, "top-level M handled") &&
      expect(m.accepted, "top-level M accepted in gameplay") &&
      expect(iggy3d::productMapMakerLiveForWindow(mapMaker, mapMakerWindow),
             "top-level M enables map maker");

  iggy3d::FrontendState creativeWorld = gameplayFrontend();
  iggy3d::ProductAppWindowState creativeWorldWindow;
  creativeWorldWindow.gameplay.gameplayActive = true;
  markCreativeDocumentWindow(creativeWorldWindow);
  creativeWorldWindow.viewport.mapMakerStatus = "map_maker_enabled";
  creativeWorldWindow.viewport.creativeFlyActive = true;
  iggy3d::creative::CreativeAppState creativeWorldApp;
  creativeWorldApp.identity.saveId = "creative_save";
  creativeWorldApp.identity.worldId = "creative_world";
  creativeWorldApp.identity.documentId = 77U;
  const iggy3d::ProductWindowTopLevelToggleResult creativeWorldM =
      iggy3d::dispatchProductWindowTopLevelToggleAction(
          creativeWorld,
          creativeWorldWindow,
          iggy3d::InputAction::MapMakerToggle,
          &settings,
          &closeRequested,
          &creativeWorldApp);
  const bool creativeWorldMOk =
      expect(creativeWorldM.handled, "top-level creative M handled") &&
      expect(!creativeWorldM.accepted, "top-level creative M rejected") &&
      expect(creativeWorldWindow.inputDevice.interactionMode ==
                 iggy3d::ProductInteractionMode::Creative,
             "top-level creative M preserves creative mode") &&
      expect(!iggy3d::productMapMakerLiveForWindow(creativeWorld,
                                                   creativeWorldWindow),
             "top-level creative M blocks map maker live state") &&
      expect(!creativeWorldWindow.viewport.creativeFlyActive,
             "top-level creative M clears stale creative fly") &&
      expect(creativeWorldWindow.viewport.mapMakerStatus ==
                 "map_maker_creative_world_active",
             "top-level creative M status");

  iggy3d::FrontendState pauseMapMaker = gameplayFrontend();
  pauseMapMaker.screen = iggy3d::FrontendScreen::Pause;
  pauseMapMaker.childScreen = iggy3d::FrontendScreen::Gameplay;
  iggy3d::ProductAppWindowState pauseMapWindow;
  pauseMapWindow.gameplay.gameplayActive = true;
  pauseMapWindow.inputDevice.interactionMode = iggy3d::ProductInteractionMode::Creative;
  const iggy3d::ProductWindowTopLevelToggleResult blockedM =
      iggy3d::dispatchProductWindowTopLevelToggleAction(
          pauseMapMaker,
          pauseMapWindow,
          iggy3d::InputAction::MapMakerToggle,
          &settings,
          &closeRequested);
  const bool blockedMOk =
      expect(blockedM.handled, "top-level pause M handled") &&
      expect(!blockedM.accepted, "top-level pause M rejected") &&
      expect(!iggy3d::productMapMakerLiveForWindow(pauseMapMaker,
                                                   pauseMapWindow),
             "top-level pause M clears map maker live state");

  return debugOk && blockedDebugOk && tuningOk && blockedTuningOk && devOk &&
         f2Ok && mOk && creativeWorldMOk && blockedMOk;
}

bool movementTuningGameplayInputIsLiveAndFocused() {
  iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window;
  window.gameplay.gameplayActive = true;
  const float originalWalk = window.gameplay.gameplayMovement.tuning.walkSpeedMetersPerSecond;

  const iggy3d::ProductMovementTuningInputResult shown =
      iggy3d::applyProductWindowMovementTuningInput(
          frontend, window, iggy3d::InputAction::MovementTuningToggle);
  const bool showOk =
      expect(shown.handled, "movement tuning toggle handled") &&
      expect(shown.accepted, "movement tuning toggle accepted") &&
      expect(window.gameplay.gameplayMovement.tuningVisible,
             "movement tuning visible after F4") &&
      expect(window.gameplay.gameplayMovement.tuningStatus == "movement_tuning_visible",
             "movement tuning visible status");

  const iggy3d::ProductMovementTuningInputResult increased =
      iggy3d::applyProductWindowMovementTuningInput(
          frontend, window, iggy3d::InputAction::MenuRight);
  const bool increaseOk =
      expect(increased.handled, "movement tuning right handled") &&
      expect(increased.accepted, "movement tuning right accepted") &&
      expect(window.gameplay.gameplayMovement.tuning.walkSpeedMetersPerSecond > originalWalk,
             "movement tuning right increases walk");

  const iggy3d::ProductMovementTuningInputResult previous =
      iggy3d::applyProductWindowMovementTuningInput(
          frontend, window, iggy3d::InputAction::MenuUp);
  const bool previousOk =
      expect(previous.handled, "movement tuning up handled") &&
      expect(previous.accepted, "movement tuning up accepted") &&
      expect(window.gameplay.gameplayMovement.tuningSelectedField ==
                 iggy3d::ProductGameplayMovementTuningField::WallRunSpeedMultiplier,
             "movement tuning up wraps to previous field");

  const iggy3d::ProductMovementTuningInputResult next =
      iggy3d::applyProductWindowMovementTuningInput(
          frontend, window, iggy3d::InputAction::MenuDown);
  const bool nextOk =
      expect(next.handled, "movement tuning down handled") &&
      expect(next.accepted, "movement tuning down accepted") &&
      expect(window.gameplay.gameplayMovement.tuningSelectedField ==
                 iggy3d::ProductGameplayMovementTuningField::WalkSpeed,
             "movement tuning down returns to walk field");

  const iggy3d::ProductMovementTuningInputResult hidden =
      iggy3d::applyProductWindowMovementTuningInput(
          frontend, window, iggy3d::InputAction::MovementTuningToggle);

  return showOk && increaseOk && previousOk && nextOk &&
         expect(hidden.handled, "movement tuning hide handled") &&
         expect(hidden.accepted, "movement tuning hide accepted") &&
         expect(!window.gameplay.gameplayMovement.tuningVisible,
                "movement tuning hidden after second F4");
}

bool movementTuningSingleLeftRightPressAppliesOneStep() {
  iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window;
  window.gameplay.gameplayActive = true;
  window.gameplay.gameplayMovement.tuningVisible = true;
  const auto& descriptor = iggy3d::productGameplayMovementTuningFieldDescriptor(
      window.gameplay.gameplayMovement.tuningSelectedField);
  const float originalWalk = window.gameplay.gameplayMovement.tuning.walkSpeedMetersPerSecond;

  const iggy3d::ProductMovementTuningInputResult right =
      iggy3d::applyProductWindowMovementTuningInput(
          frontend, window, iggy3d::InputAction::MenuRight);
  const float increasedWalk = window.gameplay.gameplayMovement.tuning.walkSpeedMetersPerSecond;
  const iggy3d::ProductMovementTuningInputResult left =
      iggy3d::applyProductWindowMovementTuningInput(
          frontend, window, iggy3d::InputAction::MenuLeft);

  return expect(right.handled, "movement tuning right single press handled") &&
         expect(right.accepted, "movement tuning right single press accepted") &&
         expectNear(increasedWalk,
                    originalWalk + descriptor.step,
                    "movement tuning right single press applies one step") &&
         expect(left.handled, "movement tuning left single press handled") &&
         expect(left.accepted, "movement tuning left single press accepted") &&
         expectNear(window.gameplay.gameplayMovement.tuning.walkSpeedMetersPerSecond,
                    originalWalk,
                    "movement tuning left single press applies one step");
}

bool movementTuningHeldRightRepeatsAfterDelay() {
  iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window;
  window.gameplay.gameplayActive = true;
  window.gameplay.gameplayMovement.tuningVisible = true;
  iggy3d::ProductMovementTuningRepeatState repeat;
  constexpr iggy3d::ProductMovementTuningRepeatPolicy policy{3U, 2U};
  const auto& descriptor = iggy3d::productGameplayMovementTuningFieldDescriptor(
      window.gameplay.gameplayMovement.tuningSelectedField);
  const float originalWalk = window.gameplay.gameplayMovement.tuning.walkSpeedMetersPerSecond;

  const iggy3d::ProductMovementTuningInputResult fresh =
      iggy3d::applyProductWindowMovementTuningInput(
          frontend, window, iggy3d::InputAction::MenuRight);
  const iggy3d::ProductMovementTuningInputResult held1 =
      iggy3d::applyProductWindowMovementTuningHeldInput(
          frontend, window, repeat, false, true, policy);
  const iggy3d::ProductMovementTuningInputResult held2 =
      iggy3d::applyProductWindowMovementTuningHeldInput(
          frontend, window, repeat, false, true, policy);
  const float beforeRepeat = window.gameplay.gameplayMovement.tuning.walkSpeedMetersPerSecond;
  const iggy3d::ProductMovementTuningInputResult held3 =
      iggy3d::applyProductWindowMovementTuningHeldInput(
          frontend, window, repeat, false, true, policy);
  const float firstRepeat = window.gameplay.gameplayMovement.tuning.walkSpeedMetersPerSecond;
  const iggy3d::ProductMovementTuningInputResult held4 =
      iggy3d::applyProductWindowMovementTuningHeldInput(
          frontend, window, repeat, false, true, policy);
  const iggy3d::ProductMovementTuningInputResult held5 =
      iggy3d::applyProductWindowMovementTuningHeldInput(
          frontend, window, repeat, false, true, policy);

  return expect(fresh.handled, "movement tuning fresh right handled") &&
         expect(fresh.accepted, "movement tuning fresh right accepted") &&
         expect(!held1.handled, "movement tuning hold frame one waits") &&
         expect(!held2.handled, "movement tuning hold frame two waits") &&
         expectNear(beforeRepeat,
                    originalWalk + descriptor.step,
                    "movement tuning hold waits through initial delay") &&
         expect(held3.handled, "movement tuning hold repeats at delay") &&
         expect(held3.accepted, "movement tuning delayed repeat accepted") &&
         expectNear(firstRepeat,
                    originalWalk + descriptor.step * 2.0F,
                    "movement tuning first repeat applies existing step") &&
         expect(!held4.handled, "movement tuning interval frame waits") &&
         expect(held5.handled, "movement tuning repeats at interval") &&
         expectNear(window.gameplay.gameplayMovement.tuning.walkSpeedMetersPerSecond,
                    originalWalk + descriptor.step * 3.0F,
                    "movement tuning interval repeat applies existing step");
}

bool movementTuningHeldRepeatReleaseResetsTiming() {
  iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window;
  window.gameplay.gameplayActive = true;
  window.gameplay.gameplayMovement.tuningVisible = true;
  iggy3d::ProductMovementTuningRepeatState repeat;
  constexpr iggy3d::ProductMovementTuningRepeatPolicy policy{2U, 1U};
  const auto& descriptor = iggy3d::productGameplayMovementTuningFieldDescriptor(
      window.gameplay.gameplayMovement.tuningSelectedField);
  const float originalWalk = window.gameplay.gameplayMovement.tuning.walkSpeedMetersPerSecond;

  (void)iggy3d::applyProductWindowMovementTuningInput(
      frontend, window, iggy3d::InputAction::MenuRight);
  (void)iggy3d::applyProductWindowMovementTuningHeldInput(
      frontend, window, repeat, false, true, policy);
  const iggy3d::ProductMovementTuningInputResult repeated =
      iggy3d::applyProductWindowMovementTuningHeldInput(
          frontend, window, repeat, false, true, policy);
  const float afterRepeat = window.gameplay.gameplayMovement.tuning.walkSpeedMetersPerSecond;
  const iggy3d::ProductMovementTuningInputResult release =
      iggy3d::applyProductWindowMovementTuningHeldInput(
          frontend, window, repeat, false, false, policy);
  const bool releaseReset = repeat.heldDirection == 0 && repeat.heldFrames == 0U;
  const iggy3d::ProductMovementTuningInputResult pressAgain =
      iggy3d::applyProductWindowMovementTuningHeldInput(
          frontend, window, repeat, false, true, policy);
  const float afterPressAgain =
      window.gameplay.gameplayMovement.tuning.walkSpeedMetersPerSecond;

  return expect(repeated.handled, "movement tuning held repeat before release") &&
         expectNear(afterRepeat,
                    originalWalk + descriptor.step * 2.0F,
                    "movement tuning held repeat applies second step") &&
         expect(!release.handled, "movement tuning release emits no repeat") &&
         expect(releaseReset, "movement tuning release resets repeat state") &&
         expect(!pressAgain.handled,
                "movement tuning press after release waits before repeat") &&
         expectNear(afterPressAgain,
                    afterRepeat,
                    "movement tuning press after release does not repeat early");
}

bool movementTuningHeldConflictDoesNotAdjust() {
  iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window;
  window.gameplay.gameplayActive = true;
  window.gameplay.gameplayMovement.tuningVisible = true;
  iggy3d::ProductMovementTuningRepeatState repeat;
  constexpr iggy3d::ProductMovementTuningRepeatPolicy policy{1U, 1U};
  const float originalWalk = window.gameplay.gameplayMovement.tuning.walkSpeedMetersPerSecond;

  const iggy3d::ProductMovementTuningInputResult conflict =
      iggy3d::applyProductWindowMovementTuningHeldInput(
          frontend, window, repeat, true, true, policy);
  const iggy3d::ProductMovementTuningInputResult rightAfterConflict =
      iggy3d::applyProductWindowMovementTuningHeldInput(
          frontend, window, repeat, false, true, policy);

  return expect(!conflict.handled,
                "movement tuning left+right conflict emits no repeat") &&
         expect(repeat.heldDirection == 1 && repeat.heldFrames == 1U,
                "movement tuning right restarts after conflict") &&
         expect(!rightAfterConflict.handled,
                "movement tuning right after conflict waits one frame") &&
         expectNear(window.gameplay.gameplayMovement.tuning.walkSpeedMetersPerSecond,
                    originalWalk,
                    "movement tuning conflict does not adjust value");
}

bool movementTuningHeldRepeatBlockedByMenuSurface() {
  iggy3d::FrontendState frontend = gameplayFrontend();
  frontend.screen = iggy3d::FrontendScreen::Pause;
  frontend.childScreen = iggy3d::FrontendScreen::Gameplay;
  iggy3d::ProductAppWindowState window;
  window.gameplay.gameplayActive = true;
  window.gameplay.gameplayMovement.tuningVisible = true;
  iggy3d::ProductMovementTuningRepeatState repeat;
  repeat.heldDirection = 1;
  repeat.heldFrames = 12U;
  constexpr iggy3d::ProductMovementTuningRepeatPolicy policy{1U, 1U};
  const float originalWalk = window.gameplay.gameplayMovement.tuning.walkSpeedMetersPerSecond;

  const iggy3d::ProductMovementTuningInputResult blocked =
      iggy3d::applyProductWindowMovementTuningHeldInput(
          frontend, window, repeat, false, true, policy);

  return expect(!blocked.handled, "movement tuning blocked hold not handled") &&
         expect(!blocked.accepted, "movement tuning blocked hold not accepted") &&
         expectNear(window.gameplay.gameplayMovement.tuning.walkSpeedMetersPerSecond,
                    originalWalk,
                    "movement tuning blocked hold does not adjust value") &&
         expect(!window.gameplay.gameplayMovement.tuningVisible,
                "movement tuning blocked hold clears stale overlay") &&
         expect(repeat.heldDirection == 0 && repeat.heldFrames == 0U,
                "movement tuning blocked hold resets repeat state");
}

bool keyboardMenuLeftRightRemainEdgeTriggered() {
  iggy3d::KeyboardInputState keyboard;
  iggy3d::KeyboardMenuInputSample sample;
  sample.rightDown = true;
  const iggy3d::InputAction first =
      iggy3d::recordKeyboardMenuAction(keyboard, sample);
  const iggy3d::InputAction held =
      iggy3d::recordKeyboardMenuAction(keyboard, sample);
  sample.rightDown = false;
  const iggy3d::InputAction released =
      iggy3d::recordKeyboardMenuAction(keyboard, sample);
  sample.leftDown = true;
  const iggy3d::InputAction left =
      iggy3d::recordKeyboardMenuAction(keyboard, sample);

  return expect(first == iggy3d::InputAction::MenuRight,
                "keyboard menu right first edge emits") &&
         expect(held == iggy3d::InputAction::None,
                "keyboard menu right hold remains edge-triggered") &&
         expect(released == iggy3d::InputAction::None,
                "keyboard menu right release emits nothing") &&
         expect(left == iggy3d::InputAction::MenuLeft,
                "keyboard menu left fresh edge still emits");
}

bool movementTuningToggleIgnoresFrontendBlockedSurfaces() {
  struct BlockedSurface {
    iggy3d::FrontendScreen screen;
    iggy3d::FrontendScreen childScreen;
    const char* label;
  };
  constexpr BlockedSurface surfaces[] = {
      {iggy3d::FrontendScreen::Starter,
       iggy3d::FrontendScreen::Gameplay,
       "starter"},
      {iggy3d::FrontendScreen::Starter,
       iggy3d::FrontendScreen::NewWorld,
       "new-world"},
      {iggy3d::FrontendScreen::Starter,
       iggy3d::FrontendScreen::DeleteConfirm,
       "delete-confirm"},
      {iggy3d::FrontendScreen::Starter,
       iggy3d::FrontendScreen::ExitConfirm,
       "exit-confirm"},
      {iggy3d::FrontendScreen::Settings,
       iggy3d::FrontendScreen::Pause,
       "pause-settings"},
      {iggy3d::FrontendScreen::Pause,
       iggy3d::FrontendScreen::Gameplay,
       "pause"},
      {iggy3d::FrontendScreen::DevOverlay,
       iggy3d::FrontendScreen::Gameplay,
       "dev-overlay"},
  };

  bool ok = true;
  for (const BlockedSurface& surface : surfaces) {
    iggy3d::FrontendState frontend = gameplayFrontend();
    frontend.screen = surface.screen;
    frontend.childScreen = surface.childScreen;
    frontend.inputOwned = false;
    iggy3d::ProductAppWindowState window;
    window.gameplay.gameplayActive = true;
    window.gameplay.gameplayMovement.tuningVisible = true;

    const iggy3d::ProductMovementTuningInputResult blocked =
        iggy3d::applyProductWindowMovementTuningInput(
            frontend, window, iggy3d::InputAction::MovementTuningToggle);
    ok = expect(blocked.handled, surface.label) && ok;
    ok = expect(!blocked.accepted, surface.label) && ok;
    ok = expect(!window.gameplay.gameplayMovement.tuningVisible, surface.label) && ok;
    ok = expect(window.gameplay.gameplayMovement.tuningStatus ==
                    "movement_tuning_gameplay_inactive",
                surface.label) &&
         ok;
  }
  return ok;
}

bool mapMakerToggleUsesGameplayOnlyCreativeMode() {
  iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window;
  window.gameplay.gameplayActive = true;

  const iggy3d::ProductMenuActionResult enabled =
      iggy3d::applyProductGameplayMapMakerToggleAction(
          iggy3d::InputAction::MapMakerToggle,
          {frontend, window});
  const bool enabledOk =
      expect(enabled.handled, "map maker toggle handled") &&
      expect(enabled.accepted, "map maker toggle accepted") &&
      expect(frontend.screen == iggy3d::FrontendScreen::Gameplay,
             "map maker toggle keeps gameplay screen") &&
      expect(window.inputDevice.interactionMode == iggy3d::ProductInteractionMode::Creative,
             "map maker toggle enters creative interaction mode") &&
      expect(liveSurface(frontend, window).inputOwner == iggy3d::MenuOwner::Gameplay,
             "map maker toggle keeps gameplay owner") &&
      expect(!liveSurface(frontend, window).gameplayInputSuppressed,
             "map maker toggle does not suppress gameplay input") &&
      expect(iggy3d::productInputOwnerFor(frontend, window) ==
                 iggy3d::MenuOwner::Gameplay,
             "map maker toggle owner derives as gameplay") &&
      expect(iggy3d::productMapMakerLiveForWindow(frontend, window),
             "map maker live derives from creative gameplay") &&
      expect(window.viewport.mapMakerStatus == "map_maker_enabled",
             "map maker enabled status") &&
      expect(frontend.status == "map_maker_enabled",
             "map maker frontend status");

  const iggy3d::ProductMenuActionResult disabled =
      iggy3d::applyProductGameplayMapMakerToggleAction(
          iggy3d::InputAction::MapMakerToggle,
          {frontend, window});
  const bool disabledOk =
      expect(disabled.handled, "map maker disable handled") &&
      expect(disabled.accepted, "map maker disable accepted") &&
      expect(frontend.screen == iggy3d::FrontendScreen::Gameplay,
             "map maker disable keeps gameplay screen") &&
      expect(window.inputDevice.interactionMode == iggy3d::ProductInteractionMode::Player,
             "map maker disable returns to player interaction mode") &&
      expect(liveSurface(frontend, window).inputOwner == iggy3d::MenuOwner::Gameplay,
             "map maker disable keeps gameplay owner") &&
      expect(!iggy3d::productMapMakerLiveForWindow(frontend, window),
             "map maker live false in player mode") &&
      expect(window.viewport.mapMakerStatus == "map_maker_disabled",
             "map maker disabled status") &&
      expect(!window.viewport.creativeFlyActive,
             "map maker disable clears creative fly active");

  iggy3d::FrontendState creativeWorld = gameplayFrontend();
  iggy3d::ProductAppWindowState creativeWorldWindow;
  creativeWorldWindow.gameplay.gameplayActive = true;
  markCreativeDocumentWindow(creativeWorldWindow);
  creativeWorldWindow.viewport.mapMakerStatus = "map_maker_enabled";
  creativeWorldWindow.viewport.creativeFlyActive = true;
  iggy3d::creative::CreativeAppState creativeWorldApp;
  creativeWorldApp.identity.saveId = "creative_save";
  creativeWorldApp.identity.worldId = "creative_world";
  creativeWorldApp.identity.documentId = 91U;
  const iggy3d::ProductMenuActionResult creativeWorldBlocked =
      iggy3d::applyProductGameplayMapMakerToggleAction(
          iggy3d::InputAction::MapMakerToggle,
          {creativeWorld, creativeWorldWindow, &creativeWorldApp});
  const bool creativeWorldBlockedOk =
      expect(creativeWorldBlocked.handled,
             "creative world map maker toggle handled") &&
      expect(!creativeWorldBlocked.accepted,
             "creative world map maker toggle rejected") &&
      expect(creativeWorldWindow.inputDevice.interactionMode ==
                 iggy3d::ProductInteractionMode::Creative,
             "creative world preserves creative interaction mode") &&
      expect(!iggy3d::productMapMakerLiveForWindow(creativeWorld,
                                                   creativeWorldWindow),
             "creative world blocks map maker live") &&
      expect(!creativeWorldWindow.viewport.creativeFlyActive,
             "creative world clears stale creative fly") &&
      expect(creativeWorldWindow.viewport.mapMakerStatus ==
                 "map_maker_creative_world_active",
             "creative world map maker status") &&
      expect(creativeWorld.status ==
                 "map_maker_toggle_creative_world_active",
             "creative world frontend status");

  iggy3d::FrontendState starter = starterFrontend();
  iggy3d::ProductAppWindowState inactiveWindow;
  const iggy3d::ProductMenuActionResult ignored =
      iggy3d::applyProductGameplayMapMakerToggleAction(
          iggy3d::InputAction::MapMakerToggle,
          {starter, inactiveWindow});
  const bool ignoredOk =
      expect(ignored.handled, "inactive map maker toggle handled") &&
      expect(!ignored.accepted, "inactive map maker toggle rejected") &&
      expect(inactiveWindow.inputDevice.interactionMode ==
                 iggy3d::ProductInteractionMode::Player,
             "inactive map maker preserves player mode") &&
      expect(inactiveWindow.viewport.mapMakerStatus == "map_maker_gameplay_inactive",
             "inactive map maker status");

  iggy3d::FrontendState pause = gameplayFrontend();
  pause.screen = iggy3d::FrontendScreen::Pause;
  pause.childScreen = iggy3d::FrontendScreen::Gameplay;
  iggy3d::ProductAppWindowState blockedWindow;
  blockedWindow.gameplay.gameplayActive = true;
  blockedWindow.inputDevice.interactionMode = iggy3d::ProductInteractionMode::Creative;
  blockedWindow.viewport.creativeFlyActive = true;
  const iggy3d::ProductMenuActionResult blocked =
      iggy3d::applyProductGameplayMapMakerToggleAction(
          iggy3d::InputAction::MapMakerToggle,
          {pause, blockedWindow});
  const bool blockedOk =
      expect(blocked.handled, "pause map maker toggle handled") &&
      expect(!blocked.accepted, "pause map maker toggle rejected") &&
      expect(blockedWindow.inputDevice.interactionMode ==
                 iggy3d::ProductInteractionMode::Player,
             "pause map maker toggle returns player mode") &&
      expect(!iggy3d::productMapMakerLiveForWindow(pause, blockedWindow),
             "pause map maker live remains false") &&
      expect(!blockedWindow.viewport.creativeFlyActive,
             "pause map maker toggle clears creative fly") &&
      expect(blockedWindow.viewport.mapMakerStatus == "map_maker_gameplay_inactive",
             "pause map maker inactive status");

  iggy3d::ProductAppWindowState staleWindow;
  staleWindow.gameplay.gameplayActive = true;
  staleWindow.inputDevice.interactionMode = iggy3d::ProductInteractionMode::Player;
  const bool staleCacheOk =
      expect(!iggy3d::productMapMakerLiveForWindow(frontend, staleWindow),
             "player mode is not map maker live");

  return enabledOk && disabledOk && creativeWorldBlockedOk && ignoredOk &&
         blockedOk && staleCacheOk;
}

bool mapMakerToggleRoutesAsGameplayOwnedInput() {
  iggy3d::InputRoutingContext gameplayContext;
  gameplayContext.owners.gameplay = true;
  const iggy3d::InputRoutingResult gameplay =
      iggy3d::routeInputAction(gameplayContext,
                               iggy3d::InputAction::MapMakerToggle);

  iggy3d::InputRoutingContext pauseContext;
  pauseContext.owners.pause = true;
  const iggy3d::InputRoutingResult pause =
      iggy3d::routeInputAction(pauseContext,
                               iggy3d::InputAction::MapMakerToggle);

  return expect(iggy3d::inputActionGroup(iggy3d::InputAction::MapMakerToggle) ==
                    iggy3d::InputActionGroup::Player,
                "map maker toggle is gameplay input group") &&
         expect(iggy3d::inputActionFeatureName(
                    iggy3d::InputAction::MapMakerToggle) == "map_maker",
                "map maker toggle feature metadata") &&
         expect(iggy3d::inputActionOwnerName(
                    iggy3d::InputAction::MapMakerToggle) == "gameplay",
                "map maker toggle owner metadata") &&
         expect(iggy3d::inputActionHandledBeforeMenu(
                    iggy3d::InputAction::MapMakerToggle),
                "map maker toggle is pre-menu gameplay action") &&
         expect(iggy3d::inputActionKeepsGameplayActive(
                    iggy3d::InputAction::MapMakerToggle),
                "map maker toggle keeps gameplay active") &&
         expect(iggy3d::inputActionFeatureName(
                    iggy3d::InputAction::MovementTuningToggle) ==
                    "movement_tuning",
                "movement tuning toggle feature metadata") &&
         expect(iggy3d::inputActionOwnerName(
                    iggy3d::InputAction::DevToggle) == "overlay",
                "dev toggle owner metadata") &&
         expect(gameplay.owner == iggy3d::MenuOwner::Gameplay,
                "map maker gameplay route owner") &&
         expect(gameplay.accepted, "map maker accepted by gameplay owner") &&
         expect(!pause.accepted, "map maker rejected by pause owner");
}

bool gameplaySettingsAdjustMovementTuningLive() {
  iggy3d::FrontendState frontend = starterFrontend();
  frontend.childScreen = iggy3d::FrontendScreen::Settings;
  iggy3d::ProductAppWindowState window;
  iggy3d::FrontendSettingsTab tab = iggy3d::FrontendSettingsTab::Gameplay;
  const float originalWalk = window.gameplay.gameplayMovement.tuning.walkSpeedMetersPerSecond;

  const iggy3d::ProductMenuActionResult increased =
      iggy3d::applyProductSettingsMenuAction(
          iggy3d::InputAction::MenuRight,
          {frontend, tab, window});
  const bool increasedOk =
      expect(increased.handled, "movement tuning right handled") &&
      expect(increased.accepted, "movement tuning right accepted") &&
      expect(window.gameplay.gameplayMovement.tuning.walkSpeedMetersPerSecond > originalWalk,
             "movement tuning increased walk speed") &&
      expect(window.gameplay.gameplayMovement.tuningStatus == "movement_tuning_adjusted",
             "movement tuning adjusted status");

  const iggy3d::ProductMenuActionResult cycled =
      iggy3d::applyProductSettingsMenuAction(
          iggy3d::InputAction::MenuConfirm,
          {frontend, tab, window});
  const bool cycledOk =
      expect(cycled.handled, "movement tuning confirm handled") &&
      expect(cycled.accepted, "movement tuning confirm accepted") &&
      expect(window.gameplay.gameplayMovement.tuningSelectedField ==
                 iggy3d::ProductGameplayMovementTuningField::SprintSpeed,
             "movement tuning selected sprint field") &&
      expect(window.gameplay.gameplayMovement.tuningStatus ==
                 "movement_tuning_field_selected",
             "movement tuning selected status");

  const float originalSprint =
      window.gameplay.gameplayMovement.tuning.sprintSpeedMetersPerSecond;
  const iggy3d::ProductMenuActionResult decreased =
      iggy3d::applyProductSettingsMenuAction(
          iggy3d::InputAction::MenuLeft,
          {frontend, tab, window});
  return increasedOk && cycledOk &&
         expect(decreased.handled, "movement tuning left handled") &&
         expect(decreased.accepted, "movement tuning left accepted") &&
         expect(window.gameplay.gameplayMovement.tuning.sprintSpeedMetersPerSecond <
                    originalSprint,
                "movement tuning decreased sprint speed");
}

bool movementTuningLookValuesDriveCameraInput() {
  iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window;
  window.gameplay.gameplayActive = true;
  window.gameplay.gameplayMovement.tuning.lookSensitivity = 2.0F;
  window.gameplay.gameplayMovement.tuning.invertLookEnabled = 1.0F;
  iggy3d::FrontendSettings settings;
  settings.lookSensitivity = 0.25F;
  settings.invertLook = false;
  iggy3d::ActionState actions;
  iggy3d::recordAction(actions,
                       iggy3d::InputAction::PlayerLookX,
                       true,
                       true,
                       false,
                       1.0F);
  iggy3d::recordAction(actions,
                       iggy3d::InputAction::PlayerLookY,
                       true,
                       true,
                       false,
                       1.0F);

  const iggy3d::ProductControllerSampleInputResult result =
      iggy3d::applyProductWindowInputActions(frontend,
                                             window,
                                             nullptr,
                                             &settings,
                                             actions,
                                             "unit/look_tuning");

  return expect(result.processed, "look tuning input processed") &&
         expect(window.viewport.lookInputUsed, "look tuning used look input") &&
         expectNear(window.viewport.cameraYawDegrees,
                    12.0F,
                    "look tuning sensitivity scales yaw") &&
         expectNear(window.viewport.cameraPitchDegrees,
                    8.0F,
                    "look tuning invert flips pitch positive") &&
         expect(settings.lookSensitivity == 0.25F,
                "look tuning leaves settings object unchanged") &&
         expect(!settings.invertLook,
                "look tuning leaves invert setting unchanged");
}

bool editorOwnedInputClearsRetainedGroundVelocity() {
  iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window = editingWindow();
  window.inputDevice.interactionMode = iggy3d::ProductInteractionMode::Creative;
  window.gameplay.gameplayMovement.groundVelocityX = 1.5F;
  window.gameplay.gameplayMovement.groundVelocityZ = -2.0F;
  window.gameplay.gameplayMovement.horizontalSpeedMetersPerSecond = 2.5F;
  window.gameplay.gameplayMovement.state =
      iggy3d::ProductGameplayMovementState::MovingGrounded;
  window.gameplay.gameplayWallRun.active = true;
  window.gameplay.gameplayWallRun.status = "wall_run_active";
  window.gameplay.gameplayWallRun.reasonCode = "wall_run_active";
  window.gameplay.gameplayWallRun.remainingSeconds = 0.5F;
  window.gameplay.gameplayJump.coyoteSecondsRemaining = 0.08F;
  window.gameplay.gameplayJump.bufferSecondsRemaining = 0.06F;
  window.gameplay.gameplayJump.held = true;
  window.gameplay.gameplayJump.cutApplied = true;
  iggy3d::ActionState actions;
  iggy3d::recordAction(actions,
                       iggy3d::InputAction::EditorNudgeX,
                       true,
                       true,
                       false,
                       1.0F);

  const iggy3d::ProductControllerSampleInputResult result =
      iggy3d::applyProductWindowInputActions(frontend,
                                             window,
                                             nullptr,
                                             nullptr,
                                             actions,
                                             "unit/editor_velocity_clear");

  return expect(result.processed, "editor input processed") &&
         expect(result.actionApplied, "editor input applied") &&
         expect(window.creativeAuthoring.roomEditorLastOperationAccepted,
                "editor input accepted") &&
         expect(window.gameplay.gameplayMovement.groundVelocityX == 0.0F &&
                    window.gameplay.gameplayMovement.groundVelocityZ == 0.0F,
                "editor input clears retained ground velocity") &&
         expect(window.gameplay.gameplayMovement.horizontalSpeedMetersPerSecond == 0.0F,
                "editor input clears horizontal speed proof") &&
         expect(window.gameplay.gameplayMovement.state ==
                    iggy3d::ProductGameplayMovementState::IdleGrounded,
                "editor input resets movement state proof") &&
         expect(!window.gameplay.gameplayWallRun.active &&
                    window.gameplay.gameplayWallRun.remainingSeconds == 0.0F,
                "editor input clears wall-run active proof") &&
         expect(window.gameplay.gameplayJump.coyoteSecondsRemaining == 0.0F &&
                    window.gameplay.gameplayJump.bufferSecondsRemaining == 0.0F &&
                    !window.gameplay.gameplayJump.held &&
                    !window.gameplay.gameplayJump.cutApplied,
                "editor input clears jump timing");
}

}  // namespace

int main() {
  const bool passed =
      creativeClickPicksCursorAndBuildsPreviewWithoutMutation() &&
      playerClickDoesNotRunEditorPickPreview() &&
      notReadyClickDoesNotMutateEditorState() &&
      invalidClickPropagatesMousePickRejectionWithoutMutation() &&
      controllerSouthStagesThenConfirmsPlacement() &&
      controllerMoveInvalidatesPendingPreview() &&
      backCancelsPendingPreviewBeforePauseRoute() &&
      backWithoutPreviewFallsThroughPolicy() &&
      controllerEastCancelsPendingPreviewWithoutMutation() &&
      controllerSouthJumpsInGameplayPlayerMode() &&
      controllerSouthDoesNotJumpWhenFrontendBlocksGameplay() &&
      mapMakerMovementStaysGameplayOwnedAndDoesNotPause() &&
      creativeDocumentSuppressesProductControllerMovement() &&
      controllerChordToggleRecordsCreativeConsumption() &&
      controllerSouthJumpsFromClamberedWallTop() &&
      starterDeleteButtonOpensSelectableBrowser() &&
      starterHitTestUsesCanonicalActionRows() &&
      pauseHitTestUsesPauseActionRows() &&
      pauseMouseClickResumesBeforeCreativeOverlayInput() &&
      pauseMouseClickResumesInActiveCreativeWorld() &&
      pauseSettingsConfirmOpensSettingsPanel() &&
      pauseSettingsInputDispatchRoutesToSettings() &&
      starterSettingsInputDispatchRoutesToSettings() &&
      pauseInputDispatchRoutesToPauseHandler() &&
      devOverlayInputDispatchRoutesToDevToolsHandler() &&
      confirmDialogDispatchDoesNotFallThroughToStarter() &&
      gameplayAndEditorSurfacesDoNotFallThroughToStarter() &&
      editorBackOpensPauseWithoutLeavingEditor() &&
      pauseSaveFlowLeavesStableFrontendStatus() &&
      pauseLoadOpensBrowserInLoadModeOverStaleResidue() &&
      pauseOpenedBrowserBackReturnsToPause() &&
      loadFromPauseReplacesTheActiveSession() &&
      childPanelHitTestsExposeMenuActions() &&
      openingMenuMouseDispatchRoutesStarterAndSettingsHits() &&
      openingMenuMouseDispatchRoutesNewWorldNavigationRows() &&
      openingMenuMouseDispatchRoutesLoadSaveRows() &&
      openingMenuMouseDispatchReportsUnhandledHitArea() &&
      menuClickNormalizationScalesWindowCoordinates() &&
      devToggleOpensAndClosesDevToolsSurfaces() &&
      debugOverlayActionTogglesRuntimeOverlaySetting() &&
      debugOverlayActionIgnoresFrontendBlockedSurfaces() &&
      collisionOverlayActionTogglesDistinctWindowState() &&
      sdlFunctionKeyEventsMapToSystemActions() &&
      sdlFunctionKeyEventsMarkKeyboardStateConsumed() &&
      topLevelToggleFunnelPreservesPolicies() &&
      movementTuningGameplayInputIsLiveAndFocused() &&
      movementTuningSingleLeftRightPressAppliesOneStep() &&
      movementTuningHeldRightRepeatsAfterDelay() &&
      movementTuningHeldRepeatReleaseResetsTiming() &&
      movementTuningHeldConflictDoesNotAdjust() &&
      movementTuningHeldRepeatBlockedByMenuSurface() &&
      keyboardMenuLeftRightRemainEdgeTriggered() &&
      movementTuningToggleIgnoresFrontendBlockedSurfaces() &&
      mapMakerToggleUsesGameplayOnlyCreativeMode() &&
      mapMakerToggleRoutesAsGameplayOwnedInput() &&
      gameplaySettingsAdjustMovementTuningLive() &&
      movementTuningLookValuesDriveCameraInput() &&
      editorOwnedInputClearsRetainedGroundVelocity();
  std::cout << "product_window_input_frame_tests="
            << (passed ? "pass" : "fail") << '\n';
  return passed ? 0 : 1;
}
