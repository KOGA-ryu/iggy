#include "app/frontend/FrontendState.hpp"
#include "app/iggy3d/ascii_room/Activation.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollision.hpp"
#include "app/iggy3d/menu/ActionHandlers.hpp"
#include "app/iggy3d/room_editor/EditingState.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"
#include "app/iggy3d/view/OpeningMenuView.hpp"
#include "app/iggy3d/window/InputFrame.hpp"
#include "app/platform/SdlWindow.hpp"
#include "app/iggy3d/automation/AutomationRoomEditing.hpp"
#include "content/assets/RoomAsset.hpp"
#include "core/math/Transform3.hpp"
#include "runtime/replay/StateHash.hpp"

#include <iostream>
#include <limits>
#include <optional>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
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
  window.gameplayActive = true;

  const iggy3d::ProductRoomEditingStartResult started =
      iggy3d::startProductRoomEditingFromAscii(smallRoomRequest());
  iggy3d::recordProductRoomEditingStart(window, started, "unit_edit_room");
  window.roomEditorCursor = {};
  window.roomEditorCursor.selectedTool = iggy3d::ProductRoomEditorTool::Wall;
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
  window.activeRoom.loaded = true;
  window.activeRoom.status = "loaded";
  window.activeRoom.reasonCode = "active_room_loaded";
  window.activeRoom.source = "unit";
  window.activeRoom.roomId = "input_frame_clamber_test";
  window.activeRoom.sourceName = "unit/input_frame_clamber_test";
  window.activeRoom.room = inputFrameClamberRoom();
  window.activeRoom.staticMeshCount = window.activeRoom.room.staticMeshes.size();
  window.activeRoom.spatialSurfaceCount =
      window.activeRoom.room.spatialSurfaces.size();
  window.activeRoom.walkableSurfaceCount = 2U;
  window.activeRoom.actorBlockerSurfaceCount = 1U;
  window.activeRoomCollision =
      iggy3d::buildProductActiveRoomCollision(window.activeRoom, session.state());
}

iggy3d::ProductAppWindowState gameplayWindow(
    std::optional<iggy3d::Session>& session) {
  iggy3d::ProductAppWindowState window;
  window.asciiRoomDraftText =
      "#######\n"
      "#.....#\n"
      "#..P..#\n"
      "#.....#\n"
      "#..$.E#\n"
      "#######\n";
  window.asciiRoomDraftRoomId = "input_frame_gameplay_room";
  window.asciiRoomDraftSourceName =
      "unit/input_frame_gameplay_room.iggyroom.txt";
  const iggy3d::ProductAsciiRoomActivationResult activated =
      iggy3d::activateProductAsciiRoomPreview(session, window);
  expect(activated.ok, "input frame gameplay activation ok");
  window.interactionMode = iggy3d::ProductInteractionMode::Player;
  return window;
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

bool creativeClickPicksCursorAndBuildsPreviewWithoutMutation() {
  const iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window = editingWindow();
  const std::uint64_t initialFloors = window.roomEditing.documentFloorCount;
  const std::uint64_t initialWalls = window.roomEditing.documentWallCount;
  const std::uint64_t initialWalkable =
      window.roomEditing.collisionWalkableSurfaceCount;
  const std::uint64_t initialBlockers =
      window.roomEditing.collisionActorBlockerSurfaceCount;

  const iggy3d::ProductWindowEditorMousePickPreviewResult result =
      iggy3d::processProductWindowEditorMousePickPreview(
          pickContext(frontend, window, clickAt(300.0F, 100.0F)));

  return expect(result.handled, "creative mouse click handled") &&
         expect(window.interactionMode == iggy3d::ProductInteractionMode::Creative,
                "edit room entry keeps creative mode") &&
         expect(result.picked, "creative mouse click picked") &&
         expect(result.previewBuilt, "creative mouse click built preview") &&
         expect(result.accepted, "creative mouse click preview accepted") &&
         expect(result.status == "room_editor_preview_ready",
                "creative mouse preview status") &&
         expect(window.inputOwner == iggy3d::MenuOwner::Editor,
                "creative mouse input owner editor") &&
         expect(window.lastInputAction ==
                    iggy3d::InputAction::EditorPreviewPlacement,
                "creative mouse semantic action") &&
         expect(window.lastInputAccepted, "creative mouse input accepted") &&
         expect(window.gameplayInputSuppressed,
                "creative mouse suppresses gameplay input") &&
         expect(window.roomEditorStatus == "room_editor_mouse_pick_mapped",
                "creative mouse pick status") &&
         expect(window.roomEditorLastOperation == "room_editor.mouse_pick",
                "creative mouse pick operation") &&
         expect(window.roomEditorCursor.gridX == 2,
                "creative mouse pick cursor x") &&
         expect(window.roomEditorCursor.gridZ == 0,
                "creative mouse pick cursor z") &&
         expect(window.roomEditorCursor.selectedTool ==
                    iggy3d::ProductRoomEditorTool::Wall,
                "creative mouse pick preserves tool") &&
         expect(window.roomEditorPreviewVisible,
                "creative mouse preview visible") &&
         expect(window.roomEditorPreviewStatus == "room_editor_preview_ready",
                "creative mouse preview receipt status") &&
         expect(window.roomEditorPreviewCandidateId == "edit_wall_1",
                "creative mouse preview candidate") &&
         expect(window.roomEditorPreviewTool == "wall",
                "creative mouse preview tool") &&
         expect(window.roomEditorPreviewGridX == 2,
                "creative mouse preview grid x") &&
         expect(window.roomEditorPreviewGridZ == 0,
                "creative mouse preview grid z") &&
         expect(window.roomEditing.documentFloorCount == initialFloors,
                "creative mouse leaves floor count") &&
         expect(window.roomEditing.documentWallCount == initialWalls,
                "creative mouse leaves wall count") &&
         expect(window.roomEditing.collisionWalkableSurfaceCount == initialWalkable,
                "creative mouse leaves walkable collision") &&
         expect(window.roomEditing.collisionActorBlockerSurfaceCount ==
                    initialBlockers,
                "creative mouse leaves blocker collision");
}

bool playerClickDoesNotRunEditorPickPreview() {
  const iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window = editingWindow();
  window.interactionMode = iggy3d::ProductInteractionMode::Player;
  const std::uint64_t initialWalls = window.roomEditing.documentWallCount;

  const iggy3d::ProductWindowEditorMousePickPreviewResult result =
      iggy3d::processProductWindowEditorMousePickPreview(
          pickContext(frontend, window, clickAt(300.0F, 100.0F)));

  return expect(!result.handled, "player mouse click not editor handled") &&
         expect(!result.picked, "player mouse click not picked") &&
         expect(!result.previewBuilt, "player mouse click no preview") &&
         expect(!result.accepted, "player mouse click not accepted") &&
         expect(result.status == "room_editor_mouse_pick_preview_mode_blocked",
                "player mouse mode-blocked status") &&
         expect(window.roomEditorCursor.gridX == 0,
                "player mouse leaves cursor x") &&
         expect(!window.roomEditorPreviewVisible,
                "player mouse leaves preview hidden") &&
         expect(window.roomEditing.documentWallCount == initialWalls,
                "player mouse leaves wall count");
}

bool notReadyClickDoesNotMutateEditorState() {
  const iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window;
  window.gameplayActive = true;
  window.interactionMode = iggy3d::ProductInteractionMode::Creative;

  const iggy3d::ProductWindowEditorMousePickPreviewResult result =
      iggy3d::processProductWindowEditorMousePickPreview(
          pickContext(frontend, window, clickAt(300.0F, 100.0F)));

  return expect(!result.handled, "not-ready mouse click not handled") &&
         expect(result.status == "room_editor_not_ready",
                "not-ready mouse click status") &&
         expect(!window.roomEditing.ready, "not-ready leaves editing off") &&
         expect(!window.roomEditorPreviewVisible,
                "not-ready leaves preview hidden");
}

bool invalidClickPropagatesMousePickRejectionWithoutMutation() {
  const iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window = editingWindow();
  const std::uint64_t initialWalls = window.roomEditing.documentWallCount;

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
         expect(window.roomEditorStatus ==
                    "room_editor_mouse_pick_invalid_input",
                "invalid mouse pick receipt status") &&
         expect(!window.lastInputAccepted, "invalid mouse input not accepted") &&
         expect(!window.roomEditorPreviewVisible,
                "invalid mouse leaves preview hidden") &&
         expect(window.roomEditing.documentWallCount == initialWalls,
                "invalid mouse leaves wall count");
}

bool controllerSouthStagesThenConfirmsPlacement() {
  const iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window = editingWindow();
  window.roomEditorCursor.gridX = 1;
  iggy3d::Session session;
  iggy3d::ProductControllerModeChordState chord;
  iggy3d::ProductControllerActionRoutingState routing;
  const std::uint64_t initialWalls = window.roomEditing.documentWallCount;

  const iggy3d::GamepadControllerActionSample south =
      iggy3d::productControllerActionSampleForControl(
          iggy3d::ProductControllerControl::SouthButton);
  const iggy3d::ProductControllerSampleInputResult staged =
      iggy3d::processProductControllerActionSample(
          {frontend, window, &session, chord, routing, nullptr, "unit"}, south);

  const bool firstOk =
      expect(staged.actionApplied, "first south action applied") &&
      expect(staged.actionAccepted, "first south stages accepted preview") &&
      expect(window.lastInputAction == iggy3d::InputAction::EditorPlace,
             "first south routes to editor place") &&
      expect(window.roomEditorStatus == "room_editor_preview_ready",
             "first south stages preview") &&
      expect(window.roomEditorLastOperation == "editor.place",
             "first south operation is editor place") &&
      expect(window.roomEditorPreviewActive, "first south preview pending") &&
      expect(window.roomEditorPreviewVisible, "first south preview visible") &&
      expect(window.roomEditorPreviewCandidateId == "edit_wall_1",
             "first south candidate id") &&
      expect(window.roomEditing.documentWallCount == initialWalls,
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
         expect(window.lastInputAction == iggy3d::InputAction::EditorPlace,
                "second south routes to editor place") &&
         expect(window.roomEditorStatus == "room_editor_preview_confirmed",
                "second south confirm status") &&
         expect(window.roomEditorLastOperation == "editor.place",
                "second south operation is editor place") &&
         expect(!window.roomEditorPreviewActive,
                "second south clears pending preview") &&
         expect(!window.roomEditorPreviewVisible,
                "second south hides preview") &&
         expect(window.roomEditorLastPrimitiveId == "edit_wall_1",
                "second south primitive id") &&
         expect(window.roomEditing.documentWallCount == initialWalls + 1U,
                "second south mutates walls") &&
         expect(window.roomEditingLastOperation == "editor.place",
                "second south editing operation") &&
         expect(window.roomEditingLastOperationAccepted,
                "second south editing accepted");
}

bool controllerMoveInvalidatesPendingPreview() {
  const iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window = editingWindow();
  iggy3d::Session session;
  iggy3d::ProductControllerModeChordState chord;
  iggy3d::ProductControllerActionRoutingState routing;
  const std::uint64_t initialWalls = window.roomEditing.documentWallCount;

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
         expect(window.lastInputAction == iggy3d::InputAction::EditorNudgeX,
                "dpad right routes to nudge x") &&
         expect(window.roomEditorCursor.gridX == 1,
                "dpad right moves cursor") &&
         expect(window.roomEditorStatus == "room_editor_cursor_moved",
                "dpad move cursor status") &&
         expect(!window.roomEditorPreviewActive,
                "dpad move clears pending preview") &&
         expect(!window.roomEditorPreviewVisible,
                "dpad move hides preview") &&
         expect(window.roomEditing.documentWallCount == initialWalls,
                "dpad move does not mutate walls");
}

bool backCancelsPendingPreviewBeforePauseRoute() {
  iggy3d::ProductAppWindowState window = editingWindow();
  const std::uint64_t initialWalls = window.roomEditing.documentWallCount;
  window.roomEditorCursor.gridX = 1;

  const iggy3d::ProductRoomEditorPreviewInputResult staged =
      iggy3d::applyProductRoomEditorPreviewInputAction(
          window, iggy3d::InputAction::EditorPlace);
  const bool stageOk =
      expect(staged.ok, "back test stages preview") &&
      expect(window.roomEditorPreviewActive, "back test preview active") &&
      expect(window.roomEditing.documentWallCount == initialWalls,
             "back test stage leaves walls");

  const bool cancelled =
      iggy3d::cancelProductRoomEditorPendingPreviewFromBack(window);

  return stageOk && expect(cancelled, "back cancels pending preview") &&
         expect(window.inputOwner == iggy3d::MenuOwner::Editor,
                "back cancel owner editor") &&
         expect(window.lastInputAction == iggy3d::InputAction::EditorCancelPreview,
                "back cancel records editor cancel action") &&
         expect(window.lastInputAccepted, "back cancel input accepted") &&
         expect(window.gameplayInputSuppressed,
                "back cancel suppresses gameplay input") &&
         expect(window.roomEditorStatus == "room_editor_preview_cancelled",
                "back cancel status") &&
         expect(window.roomEditorLastOperation == "editor.preview_cancel",
                "back cancel operation") &&
         expect(!window.roomEditorPreviewActive,
                "back cancel clears pending preview") &&
         expect(!window.roomEditorPreviewVisible, "back cancel hides preview") &&
         expect(window.roomEditing.documentWallCount == initialWalls,
                "back cancel leaves walls");
}

bool backWithoutPreviewFallsThroughPolicy() {
  iggy3d::ProductAppWindowState window = editingWindow();
  const std::uint64_t initialWalls = window.roomEditing.documentWallCount;

  const bool cancelled =
      iggy3d::cancelProductRoomEditorPendingPreviewFromBack(window);

  return expect(!cancelled, "back without preview not consumed") &&
         expect(window.roomEditing.ready, "back without preview keeps editor ready") &&
         expect(!window.roomEditorPreviewActive,
                "back without preview leaves preview inactive") &&
         expect(window.roomEditing.documentWallCount == initialWalls,
                "back without preview leaves walls");
}

bool controllerEastCancelsPendingPreviewWithoutMutation() {
  const iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window = editingWindow();
  window.roomEditorCursor.gridX = 1;
  iggy3d::Session session;
  iggy3d::ProductControllerModeChordState chord;
  iggy3d::ProductControllerActionRoutingState routing;
  const std::uint64_t initialWalls = window.roomEditing.documentWallCount;

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
         expect(window.lastInputAction == iggy3d::InputAction::EditorCancelPreview,
                "east cancel routes to editor cancel preview") &&
         expect(window.roomEditorStatus == "room_editor_preview_cancelled",
                "east cancel status") &&
         expect(window.roomEditorLastOperation == "editor.preview_cancel",
                "east cancel operation") &&
         expect(!window.roomEditorPreviewActive,
                "east cancel clears pending preview") &&
         expect(!window.roomEditorPreviewVisible, "east cancel hides preview") &&
         expect(window.roomEditing.documentWallCount == initialWalls,
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
         expect(window.inputOwner == iggy3d::MenuOwner::Gameplay,
                "gameplay south owner gameplay") &&
         expect(window.lastInputAccepted, "gameplay south input accepted") &&
         expect(!window.gameplayInputSuppressed,
                "gameplay south does not suppress gameplay") &&
         expect(window.lastInputAction == iggy3d::InputAction::PlayerJump,
                "gameplay south routes to jump") &&
         expect(window.controllerActionInputAction == "game.jump",
                "gameplay south records jump input action") &&
         expect(window.gameplayJumpRequested,
                "gameplay south requests jump") &&
         expect(window.gameplayJumpAccepted, "gameplay south accepts jump") &&
         expect(window.gameplayJumpActive, "gameplay south jump remains active") &&
         expect(window.gameplayJumpStatus == "airborne",
                "gameplay south jump airborne status") &&
         expect(window.gameplayJumpReasonCode == "gameplay_jump_airborne",
                "gameplay south jump airborne reason");
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
         expect(window.controllerActionInputAction == "game.jump",
                "wall top second south records jump action") &&
         expect(window.lastInputAction == iggy3d::InputAction::PlayerJump,
                "wall top second south routes to jump") &&
         expect(jumped.actionApplied, "wall top second south action applied") &&
         expect(window.gameplayJumpRequested, "wall top jump requested") &&
         expect(window.gameplayJumpAccepted, "wall top jump accepted") &&
         expect(window.gameplayJumpActive, "wall top jump active") &&
         expect(window.gameplayJumpStatus == "airborne",
                "wall top jump airborne") &&
         expect(window.gameplayJumpReasonCode == "gameplay_jump_airborne",
                "wall top jump reason") &&
         expect(window.gameplayJumpGroundY == clamberTopY,
                "wall top jump uses clamber top as ground") &&
         expect(finalY > clamberTopY, "wall top jump raises player");
}

bool starterDeleteButtonOpensDeleteConfirmation() {
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

  return expect(result.handled, "starter delete handled") &&
         expect(result.accepted, "starter delete accepted") &&
         expect(frontend.childScreen == iggy3d::FrontendScreen::DeleteConfirm,
                "starter delete opens confirmation") &&
         expect(frontend.selectedAction == iggy3d::FrontendAction::Delete,
                "starter delete selected action") &&
         expect(window.saveDeleteConfirmationOpen,
                "starter delete confirmation open") &&
         expect(window.saveDeleteCandidateId == "save_unit",
                "starter delete candidate id") &&
         expect(window.selectedProductSaveId == "save_unit",
                "starter delete selected save") &&
         expect(!closeRequested, "starter delete does not close app");
}

bool starterHitTestUsesCanonicalActionRows() {
  const iggy3d::FrontendState frontend = starterFrontend();
  const iggy3d::OpeningMenuHitTestResult deleteHit =
      iggy3d::openingMenuActionAt(frontend, 62.0F, 306.0F);
  const iggy3d::OpeningMenuHitTestResult exitHit =
      iggy3d::openingMenuActionAt(frontend, 62.0F, 462.0F);

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
  window.gameplayActive = true;
  bool closeRequested = false;
  const iggy3d::ProductMenuActionResult paused =
      iggy3d::applyProductSystemPauseMenuAction(
          iggy3d::InputAction::SystemPause,
          {frontend, window, closeRequested});

  const iggy3d::OpeningMenuHitTestResult resumeHit =
      iggy3d::openingMenuActionAt(frontend, 62.0F, 150.0F);
  const iggy3d::OpeningMenuHitTestResult settingsHit =
      iggy3d::openingMenuActionAt(frontend, 62.0F, 462.0F);

  return expect(paused.handled, "system pause handled") &&
         expect(paused.accepted, "system pause accepted") &&
         expect(frontend.screen == iggy3d::FrontendScreen::Pause,
                "system pause opens pause screen") &&
         expect(resumeHit.hit, "pause resume row hit") &&
         expect(resumeHit.action == iggy3d::FrontendAction::Resume,
                "pause first row is resume") &&
         expect(settingsHit.hit, "pause settings row hit") &&
         expect(settingsHit.area == iggy3d::OpeningMenuHitArea::StarterAction,
                "pause settings row uses menu action area") &&
         expect(settingsHit.action == iggy3d::FrontendAction::Settings,
                "pause settings row maps to settings");
}

bool pauseSettingsConfirmOpensSettingsPanel() {
  iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window;
  window.gameplayActive = true;
  bool closeRequested = false;
  iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::None;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppOptions options;

  (void)iggy3d::applyProductSystemPauseMenuAction(
      iggy3d::InputAction::SystemPause, {frontend, window, closeRequested});
  frontend.selectedAction = iggy3d::FrontendAction::Settings;
  const iggy3d::ProductMenuActionResult opened =
      iggy3d::applyProductPauseMenuAction(
          iggy3d::InputAction::MenuConfirm,
          {frontend, options, settingsTab, activeSession, window, closeRequested});

  return expect(opened.handled, "pause settings confirm handled") &&
         expect(opened.accepted, "pause settings confirm accepted") &&
         expect(frontend.screen == iggy3d::FrontendScreen::Settings,
                "pause settings confirm opens settings screen") &&
         expect(frontend.childScreen == iggy3d::FrontendScreen::Pause,
                "pause settings preserves pause parent") &&
         expect(settingsTab == iggy3d::FrontendSettingsTab::Input,
                "pause settings starts on input tab") &&
         expect(window.inputOwner == iggy3d::MenuOwner::Settings,
                "pause settings input owner");
}

bool childPanelHitTestsExposeMenuActions() {
  iggy3d::FrontendState frontend = starterFrontend();
  frontend.childScreen = iggy3d::FrontendScreen::NewWorld;
  const iggy3d::OpeningMenuHitTestResult createHit =
      iggy3d::openingMenuActionAt(frontend, 452.0F, 508.0F);
  const iggy3d::OpeningMenuHitTestResult newWorldBackHit =
      iggy3d::openingMenuActionAt(frontend, 850.0F, 508.0F);
  const iggy3d::OpeningMenuHitTestResult newWorldPreviousHit =
      iggy3d::openingMenuActionAt(frontend, 452.0F, 556.0F);
  const iggy3d::OpeningMenuHitTestResult newWorldNextHit =
      iggy3d::openingMenuActionAt(frontend, 570.0F, 556.0F);

  frontend.childScreen = iggy3d::FrontendScreen::LoadSave;
  const iggy3d::OpeningMenuHitTestResult slotHit =
      iggy3d::openingMenuActionAt(frontend, 452.0F, 356.0F);
  const iggy3d::OpeningMenuHitTestResult loadHit =
      iggy3d::openingMenuActionAt(frontend, 452.0F, 548.0F);
  const iggy3d::OpeningMenuHitTestResult deleteHit =
      iggy3d::openingMenuActionAt(frontend, 690.0F, 548.0F);
  const iggy3d::OpeningMenuHitTestResult loadBackHit =
      iggy3d::openingMenuActionAt(frontend, 1010.0F, 548.0F);

  frontend.childScreen = iggy3d::FrontendScreen::DeleteConfirm;
  const iggy3d::OpeningMenuHitTestResult confirmDeleteHit =
      iggy3d::openingMenuActionAt(frontend, 452.0F, 508.0F);
  const iggy3d::OpeningMenuHitTestResult deleteBackHit =
      iggy3d::openingMenuActionAt(frontend, 760.0F, 508.0F);
  frontend.childScreen = iggy3d::FrontendScreen::Settings;
  const iggy3d::OpeningMenuHitTestResult settingsBackHit =
      iggy3d::openingMenuActionAt(frontend, 850.0F, 394.0F);
  frontend.childScreen = iggy3d::FrontendScreen::StarterDevTools;
  const iggy3d::OpeningMenuHitTestResult devToolsBackHit =
      iggy3d::openingMenuActionAt(frontend, 850.0F, 394.0F);

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
         expect(devToolsBackHit.hit, "dev tools back hit") &&
         expect(devToolsBackHit.area == iggy3d::OpeningMenuHitArea::DevToolsBack,
                "dev tools back area");
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
      iggy3d::openingMenuActionAt(frontend, scaled.x, scaled.y);

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
    const bool openedFlag = frontend.devToolsOpen;
    const iggy3d::MenuOwner openedOwner = window.inputOwner;
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
        expect(!frontend.devToolsOpen, "starter dev tools closed") &&
        expect(!closeRequested, "dev toggle does not close window");
    if (!starterOk) {
      return false;
    }
  }

  iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window;
  window.gameplayActive = true;
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
      expect(frontend.devToolsOpen, "gameplay dev tools open") &&
      expect(window.inputOwner == iggy3d::MenuOwner::DevTools,
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
         expect(window.inputOwner == iggy3d::MenuOwner::Gameplay,
                "gameplay dev close owner") &&
         expect(!closeRequested, "gameplay dev toggle no close");
}

bool debugOverlayActionTogglesRuntimeOverlaySetting() {
  iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window;
  window.gameplayActive = true;
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
      expect(!frontend.devToolsOpen, "debug overlay does not open dev tools") &&
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
  iggy3d::SdlWindowEventState f1f3;
  f1f3.f1Pressed = true;
  f1f3.f3Pressed = true;

  return expect(iggy3d::productWindowFunctionKeyAction(none) ==
                    iggy3d::InputAction::None,
                "no function key maps to no action") &&
         expect(iggy3d::productWindowFunctionKeyAction(f1) ==
                    iggy3d::InputAction::DevToggle,
                "F1 event maps to dev toggle") &&
         expect(iggy3d::productWindowFunctionKeyAction(f2) ==
                    iggy3d::InputAction::DevToggle,
                "F2 event maps to dev toggle") &&
	         expect(iggy3d::productWindowFunctionKeyAction(f3) ==
	                    iggy3d::InputAction::DevDebugOverlay,
	                "F3 event maps to debug overlay") &&
	         expect(iggy3d::productWindowFunctionKeyAction(f4) ==
	                    iggy3d::InputAction::MovementTuningToggle,
	                "F4 event maps to movement tuning") &&
	         expect(iggy3d::productWindowFunctionKeyAction(f1f3) ==
	                    iggy3d::InputAction::DevDebugOverlay,
	                "F3 overlay event wins over dev panel toggle");
}

bool sdlFunctionKeyEventsMarkKeyboardStateConsumed() {
  iggy3d::KeyboardInputState keyboard;
  iggy3d::SdlWindowEventState none;
  iggy3d::recordProductWindowFunctionKeyKeyboardState(keyboard, none);
  const bool noneOk =
      expect(!keyboard.devToggleWasDown, "no F-key leaves dev toggle unconsumed") &&
      expect(!keyboard.debugOverlayWasDown,
	             "no F-key leaves debug overlay unconsumed") &&
      expect(!keyboard.movementTuningToggleWasDown,
             "no F-key leaves movement tuning unconsumed");

  iggy3d::SdlWindowEventState f2;
  f2.f2Pressed = true;
  iggy3d::recordProductWindowFunctionKeyKeyboardState(keyboard, f2);
	  const bool f2Ok =
	      expect(keyboard.devToggleWasDown, "F2 event consumes dev toggle state") &&
	      expect(!keyboard.debugOverlayWasDown,
	             "F2 event does not consume debug overlay state") &&
      expect(!keyboard.movementTuningToggleWasDown,
             "F2 event does not consume movement tuning state");

  keyboard = {};
  iggy3d::SdlWindowEventState f3;
  f3.f3Pressed = true;
  iggy3d::recordProductWindowFunctionKeyKeyboardState(keyboard, f3);
	  const bool f3Ok =
	      expect(!keyboard.devToggleWasDown,
	             "F3 event does not consume dev toggle state") &&
	      expect(keyboard.debugOverlayWasDown,
	             "F3 event consumes debug overlay state") &&
      expect(!keyboard.movementTuningToggleWasDown,
             "F3 event does not consume movement tuning state");

  keyboard = {};
  iggy3d::SdlWindowEventState f4;
  f4.f4Pressed = true;
  iggy3d::recordProductWindowFunctionKeyKeyboardState(keyboard, f4);
  const bool f4Ok =
      expect(!keyboard.devToggleWasDown,
             "F4 event does not consume dev toggle state") &&
      expect(!keyboard.debugOverlayWasDown,
             "F4 event does not consume debug overlay state") &&
      expect(keyboard.movementTuningToggleWasDown,
             "F4 event consumes movement tuning state");

	  return noneOk && f2Ok && f3Ok && f4Ok;
}

bool movementTuningGameplayInputIsLiveAndFocused() {
  iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window;
  window.gameplayActive = true;
  const float originalWalk = window.gameplayMovementTuning.walkSpeedMetersPerSecond;

  const iggy3d::ProductMovementTuningInputResult shown =
      iggy3d::applyProductWindowMovementTuningInput(
          frontend, window, iggy3d::InputAction::MovementTuningToggle);
  const bool showOk =
      expect(shown.handled, "movement tuning toggle handled") &&
      expect(shown.accepted, "movement tuning toggle accepted") &&
      expect(window.gameplayMovementTuningVisible,
             "movement tuning visible after F4") &&
      expect(window.gameplayMovementTuningStatus == "movement_tuning_visible",
             "movement tuning visible status");

  const iggy3d::ProductMovementTuningInputResult increased =
      iggy3d::applyProductWindowMovementTuningInput(
          frontend, window, iggy3d::InputAction::MenuRight);
  const bool increaseOk =
      expect(increased.handled, "movement tuning right handled") &&
      expect(increased.accepted, "movement tuning right accepted") &&
      expect(window.gameplayMovementTuning.walkSpeedMetersPerSecond > originalWalk,
             "movement tuning right increases walk");

  const iggy3d::ProductMovementTuningInputResult previous =
      iggy3d::applyProductWindowMovementTuningInput(
          frontend, window, iggy3d::InputAction::MenuUp);
  const bool previousOk =
      expect(previous.handled, "movement tuning up handled") &&
      expect(previous.accepted, "movement tuning up accepted") &&
      expect(window.gameplayMovementTuningSelectedField ==
                 iggy3d::ProductGameplayMovementTuningField::DashCooldown,
             "movement tuning up wraps to previous field");

  const iggy3d::ProductMovementTuningInputResult next =
      iggy3d::applyProductWindowMovementTuningInput(
          frontend, window, iggy3d::InputAction::MenuDown);
  const bool nextOk =
      expect(next.handled, "movement tuning down handled") &&
      expect(next.accepted, "movement tuning down accepted") &&
      expect(window.gameplayMovementTuningSelectedField ==
                 iggy3d::ProductGameplayMovementTuningField::WalkSpeed,
             "movement tuning down returns to walk field");

  const iggy3d::ProductMovementTuningInputResult hidden =
      iggy3d::applyProductWindowMovementTuningInput(
          frontend, window, iggy3d::InputAction::MovementTuningToggle);

  return showOk && increaseOk && previousOk && nextOk &&
         expect(hidden.handled, "movement tuning hide handled") &&
         expect(hidden.accepted, "movement tuning hide accepted") &&
         expect(!window.gameplayMovementTuningVisible,
                "movement tuning hidden after second F4");
}

bool mapMakerToggleUsesGameplayOnlyCreativeMode() {
  iggy3d::FrontendState frontend = gameplayFrontend();
  iggy3d::ProductAppWindowState window;
  window.gameplayActive = true;
  bool closeRequested = false;
  iggy3d::FrontendSettings settings;

  const iggy3d::ProductMenuActionResult enabled =
      iggy3d::applyProductSystemPauseMenuAction(
          iggy3d::InputAction::MapMakerToggle,
          {frontend, window, closeRequested, &settings});
  const bool enabledOk =
      expect(enabled.handled, "map maker toggle handled") &&
      expect(enabled.accepted, "map maker toggle accepted") &&
      expect(window.interactionMode == iggy3d::ProductInteractionMode::Creative,
             "map maker toggle enters creative") &&
      expect(window.mapMakerActive, "map maker active") &&
      expect(window.mapMakerStatus == "map_maker_enabled",
             "map maker enabled status") &&
      expect(frontend.status == "map_maker_enabled",
             "map maker frontend status");

  const iggy3d::ProductMenuActionResult disabled =
      iggy3d::applyProductSystemPauseMenuAction(
          iggy3d::InputAction::MapMakerToggle,
          {frontend, window, closeRequested, &settings});
  const bool disabledOk =
      expect(disabled.handled, "map maker disable handled") &&
      expect(disabled.accepted, "map maker disable accepted") &&
      expect(window.interactionMode == iggy3d::ProductInteractionMode::Player,
             "map maker toggle returns player") &&
      expect(!window.mapMakerActive, "map maker inactive") &&
      expect(window.mapMakerStatus == "map_maker_disabled",
             "map maker disabled status") &&
      expect(!window.viewport.creativeFlyActive,
             "map maker disable clears creative fly active");

  iggy3d::FrontendState starter = starterFrontend();
  iggy3d::ProductAppWindowState inactiveWindow;
  const iggy3d::ProductMenuActionResult ignored =
      iggy3d::applyProductSystemPauseMenuAction(
          iggy3d::InputAction::MapMakerToggle,
          {starter, inactiveWindow, closeRequested, &settings});
  const bool ignoredOk =
      expect(ignored.handled, "inactive map maker toggle handled") &&
      expect(!ignored.accepted, "inactive map maker toggle rejected") &&
      expect(inactiveWindow.interactionMode ==
                 iggy3d::ProductInteractionMode::Player,
             "inactive map maker preserves player mode") &&
      expect(inactiveWindow.mapMakerStatus == "map_maker_gameplay_inactive",
             "inactive map maker status");

  return enabledOk && disabledOk && ignoredOk;
}

bool gameplaySettingsAdjustMovementTuningLive() {
  iggy3d::FrontendState frontend = starterFrontend();
  frontend.childScreen = iggy3d::FrontendScreen::Settings;
  iggy3d::ProductAppWindowState window;
  iggy3d::FrontendSettingsTab tab = iggy3d::FrontendSettingsTab::Gameplay;
  const float originalWalk = window.gameplayMovementTuning.walkSpeedMetersPerSecond;

  const iggy3d::ProductMenuActionResult increased =
      iggy3d::applyProductSettingsMenuAction(
          iggy3d::InputAction::MenuRight,
          {frontend, tab, window});
  const bool increasedOk =
      expect(increased.handled, "movement tuning right handled") &&
      expect(increased.accepted, "movement tuning right accepted") &&
      expect(window.gameplayMovementTuning.walkSpeedMetersPerSecond > originalWalk,
             "movement tuning increased walk speed") &&
      expect(window.gameplayMovementTuningStatus == "movement_tuning_adjusted",
             "movement tuning adjusted status");

  const iggy3d::ProductMenuActionResult cycled =
      iggy3d::applyProductSettingsMenuAction(
          iggy3d::InputAction::MenuConfirm,
          {frontend, tab, window});
  const bool cycledOk =
      expect(cycled.handled, "movement tuning confirm handled") &&
      expect(cycled.accepted, "movement tuning confirm accepted") &&
      expect(window.gameplayMovementTuningSelectedField ==
                 iggy3d::ProductGameplayMovementTuningField::SprintSpeed,
             "movement tuning selected sprint field") &&
      expect(window.gameplayMovementTuningStatus ==
                 "movement_tuning_field_selected",
             "movement tuning selected status");

  const float originalSprint =
      window.gameplayMovementTuning.sprintSpeedMetersPerSecond;
  const iggy3d::ProductMenuActionResult decreased =
      iggy3d::applyProductSettingsMenuAction(
          iggy3d::InputAction::MenuLeft,
          {frontend, tab, window});
  return increasedOk && cycledOk &&
         expect(decreased.handled, "movement tuning left handled") &&
         expect(decreased.accepted, "movement tuning left accepted") &&
         expect(window.gameplayMovementTuning.sprintSpeedMetersPerSecond <
                    originalSprint,
                "movement tuning decreased sprint speed");
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
      controllerSouthJumpsFromClamberedWallTop() &&
      starterDeleteButtonOpensDeleteConfirmation() &&
      starterHitTestUsesCanonicalActionRows() &&
      pauseHitTestUsesPauseActionRows() &&
      pauseSettingsConfirmOpensSettingsPanel() &&
      childPanelHitTestsExposeMenuActions() &&
      menuClickNormalizationScalesWindowCoordinates() &&
      devToggleOpensAndClosesDevToolsSurfaces() &&
      debugOverlayActionTogglesRuntimeOverlaySetting() &&
      sdlFunctionKeyEventsMapToSystemActions() &&
      sdlFunctionKeyEventsMarkKeyboardStateConsumed() &&
      movementTuningGameplayInputIsLiveAndFocused() &&
      mapMakerToggleUsesGameplayOnlyCreativeMode() &&
      gameplaySettingsAdjustMovementTuningLive();
  std::cout << "product_window_input_frame_tests="
            << (passed ? "pass" : "fail") << '\n';
  return passed ? 0 : 1;
}
