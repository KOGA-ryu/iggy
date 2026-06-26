#include "app/iggy3d/ProductRoomEditorCursor.hpp"

#include "app/iggy3d/ProductRoomEditingState.hpp"

#include <cmath>
#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(float a, float b) {
  return std::fabs(a - b) < 0.0001F;
}

iggy3d::ProductAsciiRoomAuthoringRequest smallRoomRequest() {
  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceText =
      "###\n"
      "#P#\n"
      "###\n";
  request.roomId = "cursor_test_room";
  request.sourceName = "unit/cursor_test_room.iggyroom.txt";
  request.emitAssetText = false;
  return request;
}

iggy3d::EditableRoomDocument documentWithExistingEditorIds() {
  iggy3d::EditableRoomDocument document;
  iggy3d::EditableRoomFloor floor;
  floor.id = "edit_floor_7";
  document.floors.push_back(floor);
  iggy3d::EditableRoomWall wall;
  wall.id = "edit_wall_12";
  document.walls.push_back(wall);
  return document;
}

bool defaultCursorStateIsStable() {
  const iggy3d::ProductRoomEditorCursorState state;
  return expect(state.gridX == 0, "default grid x") &&
         expect(state.gridZ == 0, "default grid z") &&
         expect(state.storyIndex == 0, "default story") &&
         expect(near(state.cellSizeMeters, 1.0F), "default cell") &&
         expect(state.selectedTool == iggy3d::ProductRoomEditorTool::Floor,
                "default tool") &&
         expect(state.wallDirection == iggy3d::ProductRoomEditorDirection::Up,
                "default wall direction") &&
         expect(iggy3d::productRoomEditorToolName(state.selectedTool) ==
                    "floor",
                "default tool name") &&
         expect(iggy3d::productRoomEditorDirectionName(state.wallDirection) ==
                    "up",
                "default direction name");
}

bool cursorMovementIsGridBased() {
  iggy3d::ProductRoomEditorCursorState state;
  iggy3d::ProductRoomEditorCursorResult moved =
      iggy3d::moveProductRoomEditorCursor(
          state, iggy3d::ProductRoomEditorDirection::Right);
  bool ok = expect(moved.ok, "right accepted") &&
            expect(moved.status == "room_editor_cursor_moved",
                   "right status") &&
            expect(moved.state.gridX == 1, "right x") &&
            expect(moved.state.gridZ == 0, "right z");

  moved = iggy3d::moveProductRoomEditorCursor(
      moved.state, iggy3d::ProductRoomEditorDirection::Down);
  ok = expect(moved.state.gridX == 1, "down keeps x") &&
       expect(moved.state.gridZ == 1, "down z") && ok;

  moved = iggy3d::moveProductRoomEditorCursor(
      moved.state, iggy3d::ProductRoomEditorDirection::Left);
  ok = expect(moved.state.gridX == 0, "left x") &&
       expect(moved.state.gridZ == 1, "left keeps z") && ok;

  moved = iggy3d::moveProductRoomEditorCursor(
      moved.state, iggy3d::ProductRoomEditorDirection::Up);
  return expect(moved.state.gridX == 0, "up keeps x") &&
         expect(moved.state.gridZ == 0, "up z") && ok;
}

bool toolCycleAndWallDirectionAreDeterministic() {
  iggy3d::ProductRoomEditorCursorState state;
  iggy3d::ProductRoomEditorCursorResult changed =
      iggy3d::cycleProductRoomEditorTool(state);
  bool ok = expect(changed.ok, "cycle to wall accepted") &&
            expect(changed.status == "room_editor_tool_changed",
                   "cycle status") &&
            expect(changed.state.selectedTool ==
                       iggy3d::ProductRoomEditorTool::Wall,
                   "cycled to wall");

  changed = iggy3d::cycleProductRoomEditorTool(changed.state);
  ok = expect(changed.state.selectedTool == iggy3d::ProductRoomEditorTool::Floor,
              "cycled to floor") && ok;

  changed = iggy3d::setProductRoomEditorTool(
      changed.state, iggy3d::ProductRoomEditorTool::Wall);
  ok = expect(changed.state.selectedTool == iggy3d::ProductRoomEditorTool::Wall,
              "set wall") && ok;

  changed = iggy3d::setProductRoomEditorWallDirection(
      changed.state, iggy3d::ProductRoomEditorDirection::Right);
  ok = expect(changed.ok, "set direction accepted") &&
       expect(changed.status == "room_editor_wall_direction_changed",
              "direction status") &&
       expect(changed.state.wallDirection == iggy3d::ProductRoomEditorDirection::Right,
              "direction right") &&
       ok;

  changed = iggy3d::rotateProductRoomEditorWallDirectionClockwise(changed.state);
  ok = expect(changed.state.wallDirection == iggy3d::ProductRoomEditorDirection::Down,
              "rotated right to down") &&
       ok;
  changed = iggy3d::rotateProductRoomEditorWallDirectionClockwise(changed.state);
  ok = expect(changed.state.wallDirection == iggy3d::ProductRoomEditorDirection::Left,
              "rotated down to left") &&
       ok;
  changed = iggy3d::rotateProductRoomEditorWallDirectionClockwise(changed.state);
  ok = expect(changed.state.wallDirection == iggy3d::ProductRoomEditorDirection::Up,
              "rotated left to up") &&
       ok;
  changed = iggy3d::rotateProductRoomEditorWallDirectionClockwise(changed.state);
  return expect(changed.state.wallDirection == iggy3d::ProductRoomEditorDirection::Right,
                "rotated up to right") &&
         ok;
}

bool floorCommandUsesCursorAndNextDocumentId() {
  iggy3d::ProductRoomEditorCursorState state;
  state.gridX = 2;
  state.gridZ = -3;
  state.storyIndex = 1;
  state.cellSizeMeters = 2.0F;

  const iggy3d::EditableRoomDocument document = documentWithExistingEditorIds();
  const iggy3d::ProductRoomEditorCursorResult result =
      iggy3d::buildProductRoomEditorPlaceCommand(state, &document);

  return expect(result.ok, "floor command ok") &&
         expect(result.status == "room_editor_command_built",
                "floor command status") &&
         expect(result.command.has_value(), "floor command present") &&
         expect(result.command->kind == iggy3d::RoomEditCommandKind::AddFloor,
                "floor command kind") &&
         expect(result.command->floor.id == "edit_floor_8", "floor id") &&
         expect(result.command->floor.storyIndex == 1, "floor story") &&
         expect(near(result.command->floor.centerMeters.x, 4.0F),
                "floor center x") &&
         expect(near(result.command->floor.centerMeters.y, -0.05F),
                "floor center y") &&
         expect(near(result.command->floor.centerMeters.z, -6.0F),
                "floor center z") &&
         expect(near(result.command->floor.sizeMeters.x, 2.0F),
                "floor size x") &&
         expect(near(result.command->floor.sizeMeters.y, 0.10F),
                "floor size y") &&
         expect(near(result.command->floor.sizeMeters.z, 2.0F),
                "floor size z") &&
         expect(result.command->floor.semantics.materialId == "debug_floor",
                "floor material") &&
         expect(result.command->floor.semantics.walkable,
                "floor walkable");
}

bool wallCommandUsesCursorEdgeAndNextDocumentId() {
  iggy3d::ProductRoomEditorCursorState state;
  state.gridX = 3;
  state.gridZ = 4;
  state.storyIndex = 2;
  state.cellSizeMeters = 2.0F;
  state.selectedTool = iggy3d::ProductRoomEditorTool::Wall;

  const iggy3d::EditableRoomDocument document = documentWithExistingEditorIds();
  const iggy3d::ProductRoomEditorCursorResult result =
      iggy3d::buildProductRoomEditorPlaceCommand(state, &document);

  return expect(result.ok, "wall command ok") &&
         expect(result.command.has_value(), "wall command present") &&
         expect(result.command->kind == iggy3d::RoomEditCommandKind::AddWall,
                "wall command kind") &&
         expect(result.command->wall.id == "edit_wall_13", "wall id") &&
         expect(result.command->wall.storyIndex == 2, "wall story") &&
         expect(near(result.command->wall.startMeters.x, 5.0F),
                "wall start x") &&
         expect(near(result.command->wall.startMeters.y, 0.0F),
                "wall start y") &&
         expect(near(result.command->wall.startMeters.z, 7.0F),
                "wall start z") &&
         expect(near(result.command->wall.endMeters.x, 7.0F),
                "wall end x") &&
         expect(near(result.command->wall.endMeters.y, 0.0F),
                "wall end y") &&
         expect(near(result.command->wall.endMeters.z, 7.0F),
                "wall end z") &&
         expect(near(result.command->wall.bottomY, 0.0F), "wall bottom") &&
         expect(near(result.command->wall.heightMeters, 2.5F),
                "wall height") &&
         expect(near(result.command->wall.thicknessMeters, 1.0F),
                "wall thickness") &&
         expect(result.command->wall.semantics.materialId == "debug_wall",
                "wall material") &&
         expect(result.command->wall.semantics.blocksActor,
                "wall actor blocker") &&
         expect(result.command->wall.semantics.blocksProjectile,
                "wall projectile blocker");
}

bool cursorCommandsApplyThroughProductRoomEditingState() {
  iggy3d::ProductRoomEditingState editing =
      iggy3d::startProductRoomEditingFromAscii(smallRoomRequest()).state;
  iggy3d::ProductRoomEditorCursorState state;
  state.gridX = 2;

  const iggy3d::ProductRoomEditorCursorResult floor =
      iggy3d::buildProductRoomEditorPlaceCommand(
          state, &editing.authoringSnapshot.document);
  const iggy3d::ProductRoomEditingOperationResult addFloor =
      floor.command.has_value()
          ? iggy3d::applyProductRoomEditingCommand(
                editing,
                iggy3d::ProductRoomAuthoringInputSource::Hotkey,
                *floor.command)
          : iggy3d::ProductRoomEditingOperationResult{};

  state.selectedTool = iggy3d::ProductRoomEditorTool::Wall;
  state.gridX = 3;
  const iggy3d::ProductRoomEditorCursorResult wall =
      iggy3d::buildProductRoomEditorPlaceCommand(
          state, &editing.authoringSnapshot.document);
  const iggy3d::ProductRoomEditingOperationResult addWall =
      wall.command.has_value()
          ? iggy3d::applyProductRoomEditingCommand(
                editing,
                iggy3d::ProductRoomAuthoringInputSource::Hotkey,
                *wall.command)
          : iggy3d::ProductRoomEditingOperationResult{};

  return expect(floor.ok, "floor build ok") &&
         expect(addFloor.accepted, "floor apply accepted") &&
         expect(addFloor.edit.primitiveId == "edit_floor_1",
                "floor primitive id") &&
         expect(addFloor.state.documentFloorCount == 2U,
                "floor count after add") &&
         expect(wall.ok, "wall build ok") &&
         expect(addWall.accepted, "wall apply accepted") &&
         expect(addWall.edit.primitiveId == "edit_wall_1",
                "wall primitive id") &&
         expect(editing.documentFloorCount == 2U, "final floor count") &&
         expect(editing.documentWallCount == 9U, "final wall count") &&
         expect(editing.activeRoomCollision.ready, "final collision ready") &&
         expect(editing.collisionWalkableSurfaceCount == 2U,
                "final walkable count") &&
         expect(editing.collisionActorBlockerSurfaceCount == 9U,
                "final actor blocker count");
}

bool invalidCursorRejectsWithoutEditingMutation() {
  iggy3d::ProductRoomEditingState editing =
      iggy3d::startProductRoomEditingFromAscii(smallRoomRequest()).state;
  const std::uint64_t initialFloors = editing.documentFloorCount;
  const std::uint64_t initialWalls = editing.documentWallCount;

  iggy3d::ProductRoomEditorCursorState state;
  state.cellSizeMeters = 0.0F;
  const iggy3d::ProductRoomEditorCursorResult invalid =
      iggy3d::buildProductRoomEditorPlaceCommand(
          state, &editing.authoringSnapshot.document);
  const iggy3d::ProductRoomEditorCursorResult missing =
      iggy3d::buildProductRoomEditorPlaceCommand(state, nullptr);

  return expect(!invalid.ok, "invalid cell rejected") &&
         expect(invalid.reasonCode == "room_editor_invalid_cell_size",
                "invalid cell reason") &&
         expect(!invalid.command.has_value(), "invalid has no command") &&
         expect(!missing.ok, "missing document rejected") &&
         expect(missing.reasonCode == "room_editor_document_missing",
                "missing document reason") &&
         expect(editing.documentFloorCount == initialFloors,
                "floor count unchanged") &&
         expect(editing.documentWallCount == initialWalls,
                "wall count unchanged");
}

}  // namespace

int main() {
  const bool ok = defaultCursorStateIsStable() && cursorMovementIsGridBased() &&
                  toolCycleAndWallDirectionAreDeterministic() &&
                  floorCommandUsesCursorAndNextDocumentId() &&
                  wallCommandUsesCursorEdgeAndNextDocumentId() &&
                  cursorCommandsApplyThroughProductRoomEditingState() &&
                  invalidCursorRejectsWithoutEditingMutation();
  return ok ? 0 : 1;
}
