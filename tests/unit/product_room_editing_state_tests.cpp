#include "app/iggy3d/ProductRoomEditingState.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
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
  request.roomId = "editing_state_room";
  request.sourceName = "unit/editing_state_room.iggyroom.txt";
  request.emitAssetText = false;
  return request;
}

iggy3d::EditableRoomFloor extraFloorPrimitive() {
  iggy3d::EditableRoomFloor floor;
  floor.id = "state_floor_1";
  floor.centerMeters = {2.0F, -0.05F, 0.0F};
  floor.sizeMeters = {1.0F, 0.10F, 1.0F};
  floor.semantics = iggy3d::defaultFloorSemantics("debug_floor");
  return floor;
}

iggy3d::EditableRoomWall extraWallPrimitive() {
  iggy3d::EditableRoomWall wall;
  wall.id = "state_wall_1";
  wall.startMeters = {2.0F, 0.0F, -0.5F};
  wall.endMeters = {3.0F, 0.0F, -0.5F};
  wall.bottomY = 0.0F;
  wall.heightMeters = 2.50F;
  wall.thicknessMeters = 1.0F;
  wall.semantics = iggy3d::defaultWallSemantics("debug_wall");
  return wall;
}

bool startAsciiCreatesReadyEditingState() {
  const iggy3d::ProductRoomEditingStartResult result =
      iggy3d::startProductRoomEditingFromAscii(smallRoomRequest());
  const iggy3d::ProductRoomEditingState& state = result.state;

  return expect(result.ok, "start result ok") &&
         expect(result.status == "product_room_editing_started",
                "start status") &&
         expect(result.reasonCode == "product_room_editing_started",
                "start reason") &&
         expect(result.failedStage == "none", "start failed stage") &&
         expect(result.asciiRoom.ok, "ascii editing result ok") &&
         expect(state.ready, "state ready") &&
         expect(state.status == "product_room_editing_ready",
                "state status") &&
         expect(state.reasonCode == "product_room_editing_ready",
                "state reason") &&
         expect(state.authoringSnapshot.ready, "authoring snapshot ready") &&
         expect(state.activeRoom.loaded, "active room loaded") &&
         expect(state.activeRoom.source == "editable_room",
                "active room source") &&
         expect(state.activeRoom.hasAuthoredRoom,
                "active room authored present") &&
         expect(state.activeRoomCollision.ready, "collision ready") &&
         expect(state.documentFloorCount == 1U, "document floor count") &&
         expect(state.documentWallCount == 8U, "document wall count") &&
         expect(state.activeRoomStaticMeshCount == 9U,
                "active room mesh count") &&
         expect(state.activeRoomSpatialSurfaceCount == 17U,
                "active room surface count") &&
         expect(state.activeRoomAuthoredFloorCount == 1U,
                "active authored floor count") &&
         expect(state.activeRoomAuthoredWallCount == 8U,
                "active authored wall count") &&
         expect(state.collisionQuerySurfaceCount == 17U,
                "collision query surface count") &&
         expect(state.collisionWalkableSurfaceCount == 1U,
                "collision walkable count") &&
         expect(state.collisionActorBlockerSurfaceCount == 8U,
                "collision actor blocker count") &&
         expect(state.collisionProjectileBlockerSurfaceCount == 8U,
                "collision projectile blocker count") &&
         expect(state.undoDepth == 0U, "initial undo depth") &&
         expect(state.redoDepth == 0U, "initial redo depth");
}

bool editCommandsRebuildActiveRoomCollisionAndAuthoredCounts() {
  iggy3d::ProductRoomEditingState state =
      iggy3d::startProductRoomEditingFromAscii(smallRoomRequest()).state;

  const iggy3d::ProductRoomEditingOperationResult addFloor =
      iggy3d::applyProductRoomEditingCommand(
          state,
          iggy3d::ProductRoomAuthoringInputSource::Ai,
          iggy3d::addFloorCommand(extraFloorPrimitive()));
  const iggy3d::ProductRoomEditingOperationResult addWall =
      iggy3d::applyProductRoomEditingCommand(
          state,
          iggy3d::ProductRoomAuthoringInputSource::Mouse,
          iggy3d::addWallCommand(extraWallPrimitive()));
  const iggy3d::ProductRoomEditingOperationResult deleteFloor =
      iggy3d::applyProductRoomEditingCommand(
          state,
          iggy3d::ProductRoomAuthoringInputSource::Hotkey,
          iggy3d::deleteFloorCommand("state_floor_1"));
  const iggy3d::ProductRoomEditingOperationResult deleteWall =
      iggy3d::applyProductRoomEditingCommand(
          state,
          iggy3d::ProductRoomAuthoringInputSource::Script,
          iggy3d::deleteWallCommand("state_wall_1"));

  return expect(addFloor.accepted, "add floor accepted") &&
         expect(addFloor.status == "product_room_editing_edit_applied",
                "add floor status") &&
         expect(addFloor.inputSource == iggy3d::ProductRoomAuthoringInputSource::Ai,
                "add floor source") &&
         expect(addFloor.state.documentFloorCount == 2U,
                "add floor document count") &&
         expect(addFloor.state.activeRoomAuthoredFloorCount == 2U,
                "add floor authored count") &&
         expect(addFloor.state.activeRoomSpatialSurfaceCount == 18U,
                "add floor surface count") &&
         expect(addFloor.state.collisionQuerySurfaceCount == 18U,
                "add floor collision count") &&
         expect(addWall.accepted, "add wall accepted") &&
         expect(addWall.state.documentWallCount == 9U,
                "add wall document count") &&
         expect(addWall.state.activeRoomAuthoredWallCount == 9U,
                "add wall authored count") &&
         expect(addWall.state.activeRoomSpatialSurfaceCount == 20U,
                "add wall surface count") &&
         expect(addWall.state.collisionActorBlockerSurfaceCount == 9U,
                "add wall actor blocker count") &&
         expect(deleteFloor.accepted, "delete floor accepted") &&
         expect(deleteFloor.state.documentFloorCount == 1U,
                "delete floor document count") &&
         expect(deleteFloor.state.activeRoomSpatialSurfaceCount == 19U,
                "delete floor surface count") &&
         expect(deleteWall.accepted, "delete wall accepted") &&
         expect(state.documentFloorCount == 1U, "final floor count") &&
         expect(state.documentWallCount == 8U, "final wall count") &&
         expect(state.activeRoomAuthoredFloorCount == 1U,
                "final authored floor count") &&
         expect(state.activeRoomAuthoredWallCount == 8U,
                "final authored wall count") &&
         expect(state.activeRoomStaticMeshCount == 9U,
                "final mesh count") &&
         expect(state.activeRoomSpatialSurfaceCount == 17U,
                "final surface count") &&
         expect(state.collisionQuerySurfaceCount == 17U,
                "final collision count") &&
         expect(state.undoDepth == 4U, "final undo depth") &&
         expect(state.redoDepth == 0U, "final redo depth");
}

bool undoRedoRebuildsProductEditingState() {
  iggy3d::ProductRoomEditingState state =
      iggy3d::startProductRoomEditingFromAscii(smallRoomRequest()).state;
  const iggy3d::ProductRoomEditingOperationResult add =
      iggy3d::applyProductRoomEditingCommand(
          state,
          iggy3d::ProductRoomAuthoringInputSource::Script,
          iggy3d::addFloorCommand(extraFloorPrimitive()));
  const iggy3d::ProductRoomEditingOperationResult undo =
      iggy3d::undoProductRoomEditing(
          state, iggy3d::ProductRoomAuthoringInputSource::Hotkey);
  const iggy3d::ProductRoomEditingOperationResult redo =
      iggy3d::redoProductRoomEditing(
          state, iggy3d::ProductRoomAuthoringInputSource::Hotkey);

  return expect(add.accepted, "add before undo accepted") &&
         expect(add.state.documentFloorCount == 2U,
                "add floor document count") &&
         expect(undo.accepted, "undo accepted") &&
         expect(undo.status == "product_room_editing_undo_applied",
                "undo status") &&
         expect(undo.edit.status == iggy3d::RoomEditStatus::UndoApplied,
                "undo edit status") &&
         expect(undo.state.documentFloorCount == 1U,
                "undo document floor count") &&
         expect(undo.state.activeRoomSpatialSurfaceCount == 17U,
                "undo surface count") &&
         expect(undo.state.collisionQuerySurfaceCount == 17U,
                "undo collision count") &&
         expect(undo.state.undoDepth == 0U, "undo depth after undo") &&
         expect(undo.state.redoDepth == 1U, "redo depth after undo") &&
         expect(redo.accepted, "redo accepted") &&
         expect(redo.status == "product_room_editing_redo_applied",
                "redo status") &&
         expect(redo.edit.status == iggy3d::RoomEditStatus::RedoApplied,
                "redo edit status") &&
         expect(state.documentFloorCount == 2U,
                "state floor count after redo") &&
         expect(state.activeRoomSpatialSurfaceCount == 18U,
                "state surface count after redo") &&
         expect(state.collisionQuerySurfaceCount == 18U,
                "state collision count after redo") &&
         expect(state.undoDepth == 1U, "state undo depth after redo") &&
         expect(state.redoDepth == 0U, "state redo depth after redo");
}

bool rejectedEditPreservesPreviousState() {
  iggy3d::ProductRoomEditingState state =
      iggy3d::startProductRoomEditingFromAscii(smallRoomRequest()).state;
  const iggy3d::ProductRoomEditingState before = state;
  iggy3d::EditableRoomFloor duplicate = extraFloorPrimitive();
  duplicate.id = "floor_r1_c1";

  const iggy3d::ProductRoomEditingOperationResult result =
      iggy3d::applyProductRoomEditingCommand(
          state,
          iggy3d::ProductRoomAuthoringInputSource::Script,
          iggy3d::addFloorCommand(duplicate));

  return expect(!result.accepted, "duplicate rejected") &&
         expect(result.status == "product_room_authoring_edit_rejected",
                "duplicate status") &&
         expect(result.reasonCode == "room_edit_duplicate_id",
                "duplicate reason") &&
         expect(result.edit.status == iggy3d::RoomEditStatus::DuplicateId,
                "duplicate edit status") &&
         expect(state.documentFloorCount == before.documentFloorCount,
                "state floor count preserved") &&
         expect(state.documentWallCount == before.documentWallCount,
                "state wall count preserved") &&
         expect(state.activeRoomSpatialSurfaceCount ==
                    before.activeRoomSpatialSurfaceCount,
                "state active surfaces preserved") &&
         expect(state.collisionQuerySurfaceCount ==
                    before.collisionQuerySurfaceCount,
                "state collision surfaces preserved") &&
         expect(state.undoDepth == before.undoDepth,
                "state undo depth preserved") &&
         expect(result.state.documentFloorCount == before.documentFloorCount,
                "result state floor count preserved");
}

bool invalidAsciiFailsWithoutReadyState() {
  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceText =
      "...\n"
      "...\n";
  request.roomId = "invalid_editing_state_room";
  request.sourceName = "unit/invalid_editing_state_room.iggyroom.txt";
  request.emitAssetText = false;

  const iggy3d::ProductRoomEditingStartResult result =
      iggy3d::startProductRoomEditingFromAscii(request);

  return expect(!result.ok, "invalid start rejected") &&
         expect(result.status == "ascii_room_missing_player_spawn",
                "invalid status") &&
         expect(result.reasonCode == "ascii_room_missing_player_spawn",
                "invalid reason") &&
         expect(result.failedStage == "grid", "invalid failed stage") &&
         expect(!result.state.ready, "invalid state not ready") &&
         expect(result.state.status == "ascii_room_missing_player_spawn",
                "invalid state status") &&
         expect(result.state.documentFloorCount == 0U,
                "invalid document floor count") &&
         expect(result.state.collisionQuerySurfaceCount == 0U,
                "invalid collision count");
}

}  // namespace

int main() {
  const bool ok = startAsciiCreatesReadyEditingState() &&
                  editCommandsRebuildActiveRoomCollisionAndAuthoredCounts() &&
                  undoRedoRebuildsProductEditingState() &&
                  rejectedEditPreservesPreviousState() &&
                  invalidAsciiFailsWithoutReadyState();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
