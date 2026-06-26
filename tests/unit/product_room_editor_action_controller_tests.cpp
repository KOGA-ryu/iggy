#include "app/iggy3d/ProductRoomEditorActionController.hpp"

#include <iostream>
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
  request.roomId = "action_controller_room";
  request.sourceName = "unit/action_controller_room.iggyroom.txt";
  request.emitAssetText = false;
  return request;
}

iggy3d::ActionStateEntry action(iggy3d::InputAction input,
                                float value = 1.0F) {
  return {input, true, true, false, value};
}

bool notReadyRejectsWithoutMutation() {
  const iggy3d::ProductRoomEditingState editing;
  const iggy3d::ProductRoomEditorCursorState cursor;
  const iggy3d::ProductRoomEditorActionResult result =
      iggy3d::applyProductRoomEditorAction(
          editing, cursor, action(iggy3d::InputAction::EditorNudgeX));
  return expect(!result.ok, "not ready rejected") &&
         expect(result.handled, "not ready handled") &&
         expect(result.status == "room_editor_not_ready", "not ready status") &&
         expect(result.cursor.gridX == 0, "not ready cursor unchanged") &&
         expect(!result.editing.ready, "not ready editing unchanged");
}

bool nudgeActionsMoveCursor() {
  iggy3d::ProductRoomEditingState editing =
      iggy3d::startProductRoomEditingFromAscii(smallRoomRequest()).state;
  iggy3d::ProductRoomEditorCursorState cursor;
  iggy3d::ProductRoomEditorActionResult result =
      iggy3d::applyProductRoomEditorAction(
          editing, cursor, action(iggy3d::InputAction::EditorNudgeX, 1.0F));
  bool ok = expect(result.ok, "nudge x positive accepted") &&
            expect(result.cursor.gridX == 1, "nudge x positive right") &&
            expect(result.cursor.gridZ == 0, "nudge x positive z") &&
            expect(result.status == "room_editor_cursor_moved",
                   "nudge x status");

  result = iggy3d::applyProductRoomEditorAction(
      result.editing, result.cursor,
      action(iggy3d::InputAction::EditorNudgeZ, -1.0F));
  return expect(result.ok, "nudge z negative accepted") &&
         expect(result.cursor.gridX == 1, "nudge z keeps x") &&
         expect(result.cursor.gridZ == -1, "nudge z negative up") && ok;
}

bool toolActionsChangeDeterministically() {
  iggy3d::ProductRoomEditingState editing =
      iggy3d::startProductRoomEditingFromAscii(smallRoomRequest()).state;
  iggy3d::ProductRoomEditorCursorState cursor;
  iggy3d::ProductRoomEditorActionResult result =
      iggy3d::applyProductRoomEditorAction(
          editing, cursor, action(iggy3d::InputAction::EditorNextTool));
  bool ok = expect(result.ok, "next tool accepted") &&
            expect(result.cursor.selectedTool ==
                       iggy3d::ProductRoomEditorTool::Wall,
                   "next tool to wall") &&
            expect(result.status == "room_editor_tool_changed",
                   "next tool status");

  result = iggy3d::applyProductRoomEditorAction(
      result.editing, result.cursor,
      action(iggy3d::InputAction::EditorPreviousTool));
  return expect(result.ok, "previous tool accepted") &&
         expect(result.cursor.selectedTool == iggy3d::ProductRoomEditorTool::Floor,
                "previous tool returns floor") && ok;
}

bool placeAppliesThroughEditingState() {
  iggy3d::ProductRoomEditingState editing =
      iggy3d::startProductRoomEditingFromAscii(smallRoomRequest()).state;
  iggy3d::ProductRoomEditorCursorState cursor;
  cursor.selectedTool = iggy3d::ProductRoomEditorTool::Wall;
  cursor.gridX = 2;
  const std::uint64_t initialWalls = editing.documentWallCount;

  const iggy3d::ProductRoomEditorActionResult result =
      iggy3d::applyProductRoomEditorAction(
          editing, cursor, action(iggy3d::InputAction::EditorPlace));
  return expect(result.ok, "place accepted") &&
         expect(result.status == "room_editor_command_applied",
                "place status") &&
         expect(result.operation == "editor.place", "place operation") &&
         expect(result.operationAccepted, "place operation accepted") &&
         expect(result.primitiveId == "edit_wall_1", "place primitive") &&
         expect(result.editing.documentWallCount == initialWalls + 1U,
                "wall count incremented") &&
         expect(result.editing.activeRoomCollision.ready,
                "collision rebuilt") &&
         expect(result.editing.collisionActorBlockerSurfaceCount ==
                    initialWalls + 1U,
                "actor blocker incremented");
}

bool deleteUndoRedoApplyThroughEditingState() {
  iggy3d::ProductRoomEditingState editing =
      iggy3d::startProductRoomEditingFromAscii(smallRoomRequest()).state;
  iggy3d::ProductRoomEditorCursorState cursor;
  cursor.selectedTool = iggy3d::ProductRoomEditorTool::Wall;
  cursor.gridX = 2;
  const std::uint64_t initialWalls = editing.documentWallCount;
  const iggy3d::ProductRoomEditorActionResult placed =
      iggy3d::applyProductRoomEditorAction(
          editing, cursor, action(iggy3d::InputAction::EditorPlace));
  const iggy3d::ProductRoomEditorActionResult deleted =
      iggy3d::applyProductRoomEditorAction(
          placed.editing, placed.cursor, action(iggy3d::InputAction::EditorDelete));
  const iggy3d::ProductRoomEditorActionResult undone =
      iggy3d::applyProductRoomEditorAction(
          deleted.editing, deleted.cursor, action(iggy3d::InputAction::EditorUndo));
  const iggy3d::ProductRoomEditorActionResult redone =
      iggy3d::applyProductRoomEditorAction(
          undone.editing, undone.cursor, action(iggy3d::InputAction::EditorRedo));

  return expect(placed.ok, "place setup accepted") &&
         expect(deleted.ok, "delete accepted") &&
         expect(deleted.status == "room_editor_delete_applied",
                "delete status") &&
         expect(deleted.operation == "editor.delete", "delete operation") &&
         expect(deleted.operationAccepted, "delete operation accepted") &&
         expect(deleted.primitiveId == "edit_wall_1", "delete primitive") &&
         expect(deleted.editing.documentWallCount == initialWalls,
                "delete restores wall count") &&
         expect(deleted.editing.activeRoomCollision.ready,
                "delete rebuilds collision") &&
         expect(undone.ok, "undo accepted") &&
         expect(undone.status == "room_editor_undo_applied", "undo status") &&
         expect(undone.operation == "editor.undo", "undo operation") &&
         expect(undone.primitiveId == "edit_wall_1", "undo primitive") &&
         expect(undone.editing.documentWallCount == initialWalls + 1U,
                "undo restores wall") &&
         expect(redone.ok, "redo accepted") &&
         expect(redone.status == "room_editor_redo_applied", "redo status") &&
         expect(redone.operation == "editor.redo", "redo operation") &&
         expect(redone.primitiveId == "edit_wall_1", "redo primitive") &&
         expect(redone.editing.documentWallCount == initialWalls,
                "redo deletes wall again");
}

bool deleteMissingTargetIsStable() {
  iggy3d::ProductRoomEditingState editing =
      iggy3d::startProductRoomEditingFromAscii(smallRoomRequest()).state;
  iggy3d::ProductRoomEditorCursorState cursor;
  cursor.selectedTool = iggy3d::ProductRoomEditorTool::Wall;
  cursor.gridX = 12;
  const iggy3d::ProductRoomEditorActionResult result =
      iggy3d::applyProductRoomEditorAction(
          editing, cursor, action(iggy3d::InputAction::EditorDelete));
  return expect(!result.ok, "missing delete rejected") &&
         expect(result.handled, "missing delete handled") &&
         expect(result.status == "room_editor_delete_target_not_found",
                "missing delete status") &&
         expect(result.operation == "editor.delete", "missing delete operation") &&
         expect(!result.operationAccepted, "missing delete not accepted") &&
         expect(result.primitiveId == "none", "missing delete primitive");
}

bool unsupportedActionsAreIgnored() {
  iggy3d::ProductRoomEditingState editing =
      iggy3d::startProductRoomEditingFromAscii(smallRoomRequest()).state;
  const std::uint64_t initialWalls = editing.documentWallCount;
  const iggy3d::ProductRoomEditorCursorState cursor;
  const iggy3d::ProductRoomEditorActionResult result =
      iggy3d::applyProductRoomEditorAction(
          editing, cursor, action(iggy3d::InputAction::EditorToggle));
  return expect(!result.ok, "toggle ignored") &&
         expect(result.status == "room_editor_action_not_supported",
                "toggle status") &&
         expect(result.editing.documentWallCount == initialWalls,
                "toggle leaves walls") &&
         expect(result.cursor.gridX == 0, "toggle leaves cursor");
}

}  // namespace

int main() {
  const bool ok = notReadyRejectsWithoutMutation() && nudgeActionsMoveCursor() &&
                  toolActionsChangeDeterministically() &&
                  placeAppliesThroughEditingState() &&
                  deleteUndoRedoApplyThroughEditingState() &&
                  deleteMissingTargetIsStable() &&
                  unsupportedActionsAreIgnored();
  return ok ? 0 : 1;
}
