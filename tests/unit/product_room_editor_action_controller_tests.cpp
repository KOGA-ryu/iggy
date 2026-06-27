#include "app/iggy3d/room_editor/ProductRoomEditorActionController.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/product/AutomationRoomEditing.hpp"

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

bool directToolActionsSelectWithoutMutation() {
  iggy3d::ProductRoomEditingState editing =
      iggy3d::startProductRoomEditingFromAscii(smallRoomRequest()).state;
  iggy3d::ProductRoomEditorCursorState cursor;
  const std::uint64_t initialFloors = editing.documentFloorCount;
  const std::uint64_t initialWalls = editing.documentWallCount;

  iggy3d::ProductRoomEditorActionResult result =
      iggy3d::applyProductRoomEditorAction(
          editing, cursor, action(iggy3d::InputAction::EditorSelectWallTool));
  bool ok = expect(result.ok, "direct wall tool accepted") &&
            expect(result.status == "room_editor_tool_changed",
                   "direct wall tool status") &&
            expect(result.operation == "editor.select_wall_tool",
                   "direct wall tool operation") &&
            expect(result.operationAccepted,
                   "direct wall tool operation accepted") &&
            expect(result.cursor.selectedTool == iggy3d::ProductRoomEditorTool::Wall,
                   "direct wall tool selected") &&
            expect(result.editing.documentFloorCount == initialFloors,
                   "direct wall tool leaves floors") &&
            expect(result.editing.documentWallCount == initialWalls,
                   "direct wall tool leaves walls");

  result = iggy3d::applyProductRoomEditorAction(
      result.editing, result.cursor,
      action(iggy3d::InputAction::EditorSelectFloorTool));
  return expect(result.ok, "direct floor tool accepted") &&
         expect(result.status == "room_editor_tool_changed",
                "direct floor tool status") &&
         expect(result.operation == "editor.select_floor_tool",
                "direct floor tool operation") &&
         expect(result.operationAccepted,
                "direct floor tool operation accepted") &&
         expect(result.cursor.selectedTool == iggy3d::ProductRoomEditorTool::Floor,
                "direct floor tool selected") &&
         expect(result.editing.documentFloorCount == initialFloors,
                "direct floor tool leaves floors") &&
         expect(result.editing.documentWallCount == initialWalls,
                "direct floor tool leaves walls") &&
         ok;
}

bool wallDirectionRotateActionsAreDeterministic() {
  iggy3d::ProductRoomEditingState editing =
      iggy3d::startProductRoomEditingFromAscii(smallRoomRequest()).state;
  iggy3d::ProductRoomEditorCursorState cursor;
  cursor.selectedTool = iggy3d::ProductRoomEditorTool::Wall;
  const std::uint64_t initialFloors = editing.documentFloorCount;
  const std::uint64_t initialWalls = editing.documentWallCount;

  iggy3d::ProductRoomEditorActionResult result =
      iggy3d::applyProductRoomEditorAction(
          editing, cursor,
          action(iggy3d::InputAction::EditorRotateWallDirection));
  bool ok = expect(result.ok, "rotate wall direction accepted") &&
            expect(result.status == "room_editor_wall_direction_changed",
                   "rotate wall direction status") &&
            expect(result.operation == "editor.rotate_wall_direction",
                   "rotate wall direction operation") &&
            expect(result.operationAccepted,
                   "rotate wall direction operation accepted") &&
            expect(result.cursor.wallDirection ==
                       iggy3d::ProductRoomEditorDirection::Right,
                   "rotate up to right") &&
            expect(result.editing.documentFloorCount == initialFloors,
                   "rotate leaves floors") &&
            expect(result.editing.documentWallCount == initialWalls,
                   "rotate leaves walls");

  result = iggy3d::applyProductRoomEditorAction(
      result.editing, result.cursor,
      action(iggy3d::InputAction::EditorRotateWallDirection));
  ok = expect(result.cursor.wallDirection == iggy3d::ProductRoomEditorDirection::Down,
              "rotate right to down") &&
       ok;
  result = iggy3d::applyProductRoomEditorAction(
      result.editing, result.cursor,
      action(iggy3d::InputAction::EditorRotateWallDirection));
  ok = expect(result.cursor.wallDirection == iggy3d::ProductRoomEditorDirection::Left,
              "rotate down to left") &&
       ok;
  result = iggy3d::applyProductRoomEditorAction(
      result.editing, result.cursor,
      action(iggy3d::InputAction::EditorRotateWallDirection));
  ok = expect(result.cursor.wallDirection == iggy3d::ProductRoomEditorDirection::Up,
              "rotate left to up") &&
       ok;
  return expect(result.editing.documentFloorCount == initialFloors,
                "rotation cycle leaves floors") &&
         expect(result.editing.documentWallCount == initialWalls,
                "rotation cycle leaves walls") &&
         ok;
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

bool floorDeleteUndoRedoApplyThroughEditingState() {
  iggy3d::ProductRoomEditingState editing =
      iggy3d::startProductRoomEditingFromAscii(smallRoomRequest()).state;
  iggy3d::ProductRoomEditorCursorState cursor;
  cursor.selectedTool = iggy3d::ProductRoomEditorTool::Floor;
  cursor.gridX = 2;
  const std::uint64_t initialFloors = editing.documentFloorCount;
  const std::uint64_t initialWalkable = editing.collisionWalkableSurfaceCount;
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

  return expect(placed.ok, "floor place setup accepted") &&
         expect(placed.status == "room_editor_command_applied",
                "floor place status") &&
         expect(placed.operation == "editor.place", "floor place operation") &&
         expect(placed.primitiveId == "edit_floor_1", "floor place primitive") &&
         expect(placed.editing.documentFloorCount == initialFloors + 1U,
                "floor place increments document count") &&
         expect(placed.editing.collisionWalkableSurfaceCount == initialWalkable + 1U,
                "floor place increments walkable collision count") &&
         expect(deleted.ok, "floor delete accepted") &&
         expect(deleted.status == "room_editor_delete_applied",
                "floor delete status") &&
         expect(deleted.operation == "editor.delete", "floor delete operation") &&
         expect(deleted.operationAccepted, "floor delete operation accepted") &&
         expect(deleted.primitiveId == "edit_floor_1", "floor delete primitive") &&
         expect(deleted.editing.documentFloorCount == initialFloors,
                "floor delete restores document count") &&
         expect(deleted.editing.collisionWalkableSurfaceCount == initialWalkable,
                "floor delete restores walkable collision count") &&
         expect(deleted.editing.activeRoomCollision.ready,
                "floor delete rebuilds collision") &&
         expect(undone.ok, "floor undo accepted") &&
         expect(undone.status == "room_editor_undo_applied",
                "floor undo status") &&
         expect(undone.operation == "editor.undo", "floor undo operation") &&
         expect(undone.primitiveId == "edit_floor_1", "floor undo primitive") &&
         expect(undone.editing.documentFloorCount == initialFloors + 1U,
                "floor undo restores document count") &&
         expect(undone.editing.collisionWalkableSurfaceCount == initialWalkable + 1U,
                "floor undo restores walkable collision count") &&
         expect(redone.ok, "floor redo accepted") &&
         expect(redone.status == "room_editor_redo_applied",
                "floor redo status") &&
         expect(redone.operation == "editor.redo", "floor redo operation") &&
         expect(redone.primitiveId == "edit_floor_1", "floor redo primitive") &&
         expect(redone.editing.documentFloorCount == initialFloors,
                "floor redo deletes floor again") &&
         expect(redone.editing.collisionWalkableSurfaceCount == initialWalkable,
                "floor redo restores deleted walkable collision count");
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

bool mousePickUpdatesCursorWithoutMutation() {
  iggy3d::ProductRoomEditingState editing =
      iggy3d::startProductRoomEditingFromAscii(smallRoomRequest()).state;
  iggy3d::ProductRoomEditorCursorState cursor;
  cursor.selectedTool = iggy3d::ProductRoomEditorTool::Wall;
  cursor.wallDirection = iggy3d::ProductRoomEditorDirection::Right;
  cursor.storyIndex = 1;
  const std::uint64_t initialFloors = editing.documentFloorCount;
  const std::uint64_t initialWalls = editing.documentWallCount;
  const std::uint64_t initialWalkable = editing.collisionWalkableSurfaceCount;
  const std::uint64_t initialBlockers = editing.collisionActorBlockerSurfaceCount;

  iggy3d::ProductRoomEditorMousePickRequest request;
  request.roomEditingReady = true;
  request.cursor = cursor;
  request.screenX = 732.0F;
  request.screenY = 394.0F;
  const iggy3d::ProductRoomEditorActionResult result =
      iggy3d::applyProductRoomEditorMousePick(editing, request);

  return expect(result.ok, "mouse pick accepted") &&
         expect(result.handled, "mouse pick handled") &&
         expect(result.status == "room_editor_mouse_pick_mapped",
                "mouse pick status") &&
         expect(result.reasonCode == "room_editor_mouse_pick_mapped",
                "mouse pick reason") &&
         expect(result.operation == "room_editor.mouse_pick",
                "mouse pick operation") &&
         expect(result.operationAccepted, "mouse pick accepted operation") &&
         expect(result.primitiveId == "none", "mouse pick primitive none") &&
         expect(result.cursor.gridX == 1, "mouse pick cursor x") &&
         expect(result.cursor.gridZ == 0, "mouse pick cursor z") &&
         expect(result.cursor.selectedTool == cursor.selectedTool,
                "mouse pick preserves tool") &&
         expect(result.cursor.wallDirection == cursor.wallDirection,
                "mouse pick preserves wall direction") &&
         expect(result.cursor.storyIndex == cursor.storyIndex,
                "mouse pick preserves story") &&
         expect(result.editing.documentFloorCount == initialFloors,
                "mouse pick leaves floors") &&
         expect(result.editing.documentWallCount == initialWalls,
                "mouse pick leaves walls") &&
         expect(result.editing.collisionWalkableSurfaceCount == initialWalkable,
                "mouse pick leaves walkable collision") &&
         expect(result.editing.collisionActorBlockerSurfaceCount == initialBlockers,
                "mouse pick leaves blocker collision");
}

bool mousePickRejectsInvalidWithoutMutation() {
  const iggy3d::ProductRoomEditingState notReadyEditing;
  iggy3d::ProductRoomEditorMousePickRequest request;
  request.roomEditingReady = true;
  request.cursor.gridX = 4;
  request.cursor.gridZ = 5;
  request.screenX = 640.0F;
  request.screenY = 394.0F;
  const iggy3d::ProductRoomEditorActionResult notReady =
      iggy3d::applyProductRoomEditorMousePick(notReadyEditing, request);

  iggy3d::ProductRoomEditingState editing =
      iggy3d::startProductRoomEditingFromAscii(smallRoomRequest()).state;
  const std::uint64_t initialFloors = editing.documentFloorCount;
  const std::uint64_t initialWalls = editing.documentWallCount;
  request.cursor.cellSizeMeters = 0.0F;
  const iggy3d::ProductRoomEditorActionResult invalid =
      iggy3d::applyProductRoomEditorMousePick(editing, request);

  return expect(!notReady.ok, "not ready mouse pick rejected") &&
         expect(notReady.status == "room_editor_mouse_pick_not_ready",
                "not ready mouse pick status") &&
         expect(notReady.cursor.gridX == 4, "not ready preserves x") &&
         expect(notReady.cursor.gridZ == 5, "not ready preserves z") &&
         expect(!invalid.ok, "invalid mouse pick rejected") &&
         expect(invalid.status == "room_editor_mouse_pick_invalid_cell_size",
                "invalid mouse pick status") &&
         expect(invalid.editing.documentFloorCount == initialFloors,
                "invalid mouse pick leaves floors") &&
         expect(invalid.editing.documentWallCount == initialWalls,
                "invalid mouse pick leaves walls");
}

bool previewInputBuildsAndConfirmsThroughEditingState() {
  iggy3d::ProductAppWindowState window;
  window.roomEditing =
      iggy3d::startProductRoomEditingFromAscii(smallRoomRequest()).state;
  window.roomEditorCursor.selectedTool = iggy3d::ProductRoomEditorTool::Wall;
  window.roomEditorCursor.gridX = 2;
  const std::uint64_t initialWalls = window.roomEditing.documentWallCount;
  const std::uint64_t initialBlockers =
      window.roomEditing.collisionActorBlockerSurfaceCount;

  const iggy3d::ProductRoomEditorPreviewInputResult preview =
      iggy3d::applyProductRoomEditorPreviewInputAction(
          window, iggy3d::InputAction::EditorPreviewPlacement);
  bool ok = expect(preview.ok, "preview input accepted") &&
            expect(preview.status == "room_editor_preview_ready",
                   "preview input status") &&
            expect(preview.operation == "editor.preview",
                   "preview input operation") &&
            expect(preview.primitiveId == "edit_wall_1",
                   "preview input primitive") &&
            expect(window.roomEditorPreviewActive,
                   "preview input activates preview") &&
            expect(window.roomEditorPreviewVisible,
                   "preview input visible") &&
            expect(window.roomEditing.documentWallCount == initialWalls,
                   "preview input leaves walls") &&
            expect(window.roomEditing.collisionActorBlockerSurfaceCount ==
                       initialBlockers,
                   "preview input leaves collision");

  const iggy3d::ProductRoomEditorPreviewInputResult confirmed =
      iggy3d::applyProductRoomEditorPreviewInputAction(
          window, iggy3d::InputAction::EditorConfirmPreview);
  return expect(confirmed.ok, "preview confirm accepted") &&
         expect(confirmed.status == "room_editor_preview_confirmed",
                "preview confirm status") &&
         expect(confirmed.operation == "editor.preview_confirm",
                "preview confirm operation") &&
         expect(confirmed.primitiveId == "edit_wall_1",
                "preview confirm primitive") &&
         expect(!window.roomEditorPreviewActive, "preview confirm clears active") &&
         expect(!window.roomEditorPreviewVisible, "preview confirm hidden") &&
         expect(window.roomEditing.documentWallCount == initialWalls + 1U,
                "preview confirm increments walls") &&
         expect(window.roomEditing.collisionActorBlockerSurfaceCount ==
                    initialBlockers + 1U,
                "preview confirm increments collision") &&
         expect(window.roomEditingLastOperation == "editor.preview_confirm",
                "preview confirm records room editing operation") &&
         expect(window.roomEditingLastOperationStatus ==
                    "product_room_editing_edit_applied",
                "preview confirm records edit applied") &&
         expect(window.roomEditingLastPrimitiveId == "edit_wall_1",
                "preview confirm records editing primitive") &&
         ok;
}

bool previewInputCancelAndMissingConfirmDoNotMutate() {
  iggy3d::ProductAppWindowState cancelWindow;
  cancelWindow.roomEditing =
      iggy3d::startProductRoomEditingFromAscii(smallRoomRequest()).state;
  cancelWindow.roomEditorCursor.selectedTool = iggy3d::ProductRoomEditorTool::Wall;
  cancelWindow.roomEditorCursor.gridX = 2;
  const std::uint64_t initialWalls = cancelWindow.roomEditing.documentWallCount;
  const std::uint64_t initialBlockers =
      cancelWindow.roomEditing.collisionActorBlockerSurfaceCount;

  const iggy3d::ProductRoomEditorPreviewInputResult preview =
      iggy3d::applyProductRoomEditorPreviewInputAction(
          cancelWindow, iggy3d::InputAction::EditorPreviewPlacement);
  const iggy3d::ProductRoomEditorPreviewInputResult cancelled =
      iggy3d::applyProductRoomEditorPreviewInputAction(
          cancelWindow, iggy3d::InputAction::EditorCancelPreview);

  iggy3d::ProductAppWindowState missingWindow;
  missingWindow.roomEditing =
      iggy3d::startProductRoomEditingFromAscii(smallRoomRequest()).state;
  const std::uint64_t missingWalls = missingWindow.roomEditing.documentWallCount;
  const iggy3d::ProductRoomEditorPreviewInputResult missing =
      iggy3d::applyProductRoomEditorPreviewInputAction(
          missingWindow, iggy3d::InputAction::EditorConfirmPreview);

  return expect(preview.ok, "cancel setup preview accepted") &&
         expect(cancelled.ok, "preview cancel accepted") &&
         expect(cancelled.status == "room_editor_preview_cancelled",
                "preview cancel status") &&
         expect(cancelled.operation == "editor.preview_cancel",
                "preview cancel operation") &&
         expect(!cancelWindow.roomEditorPreviewActive,
                "preview cancel clears active") &&
         expect(cancelWindow.roomEditing.documentWallCount == initialWalls,
                "preview cancel leaves walls") &&
         expect(cancelWindow.roomEditing.collisionActorBlockerSurfaceCount ==
                    initialBlockers,
                "preview cancel leaves collision") &&
         expect(cancelWindow.roomEditorLastOperation == "editor.preview_cancel",
                "preview cancel records editor operation") &&
         expect(!missing.ok, "missing preview confirm rejected") &&
         expect(missing.status == "room_editor_preview_confirm_missing",
                "missing preview confirm status") &&
         expect(missingWindow.roomEditing.documentWallCount == missingWalls,
                "missing preview confirm leaves walls") &&
         expect(!missingWindow.roomEditorPreviewActive,
                "missing preview confirm no active preview");
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
                  directToolActionsSelectWithoutMutation() &&
                  wallDirectionRotateActionsAreDeterministic() &&
                  placeAppliesThroughEditingState() &&
                  deleteUndoRedoApplyThroughEditingState() &&
                  floorDeleteUndoRedoApplyThroughEditingState() &&
                  deleteMissingTargetIsStable() &&
                  mousePickUpdatesCursorWithoutMutation() &&
                  mousePickRejectsInvalidWithoutMutation() &&
                  previewInputBuildsAndConfirmsThroughEditingState() &&
                  previewInputCancelAndMissingConfirmDoNotMutate() &&
                  unsupportedActionsAreIgnored();
  return ok ? 0 : 1;
}
