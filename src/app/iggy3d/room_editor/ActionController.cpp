#include "app/iggy3d/room_editor/ActionController.hpp"

#include <cmath>
#include <optional>
#include <string_view>

namespace iggy3d {
namespace {

ProductRoomEditorActionResult baseResult(const ProductRoomEditingState& editing,
                                         ProductRoomEditorCursorState cursor,
                                         std::string operation) {
  ProductRoomEditorActionResult result;
  result.editing = editing;
  result.cursor = cursor;
  result.operation = std::move(operation);
  return result;
}

ProductRoomEditorActionResult ignoredResult(
    const ProductRoomEditingState& editing,
    ProductRoomEditorCursorState cursor,
    std::string operation) {
  ProductRoomEditorActionResult result =
      baseResult(editing, cursor, std::move(operation));
  result.status = "room_editor_action_ignored";
  result.reasonCode = result.status;
  return result;
}

ProductRoomEditorActionResult notReadyResult(
    const ProductRoomEditingState& editing,
    ProductRoomEditorCursorState cursor,
    std::string operation) {
  ProductRoomEditorActionResult result =
      baseResult(editing, cursor, std::move(operation));
  result.handled = true;
  result.status = "room_editor_not_ready";
  result.reasonCode = result.status;
  return result;
}

ProductRoomEditorActionResult cursorResult(
    const ProductRoomEditingState& editing,
    const ProductRoomEditorCursorResult& cursor,
    std::string operation) {
  ProductRoomEditorActionResult result =
      baseResult(editing, cursor.state, std::move(operation));
  result.handled = true;
  result.ok = cursor.ok;
  result.operationAccepted = cursor.ok;
  result.status = cursor.status;
  result.reasonCode = cursor.reasonCode;
  return result;
}

ProductRoomEditorActionResult placeResult(
    const ProductRoomEditingState& editing,
    ProductRoomEditorCursorState cursor,
    const ActionStateEntry& action,
    ProductRoomAuthoringInputSource inputSource) {
  ProductRoomEditorActionResult result =
      baseResult(editing, cursor, std::string(inputActionName(action.action)));
  result.handled = true;

  const ProductRoomEditorCursorResult command =
      buildProductRoomEditorPlaceCommand(
          cursor, &editing.authoringSnapshot.document);
  result.cursor = command.state;
  if (!command.ok || !command.command.has_value()) {
    result.status = command.status;
    result.reasonCode = command.reasonCode;
    return result;
  }

  ProductRoomEditingOperationResult applied =
      applyProductRoomEditingCommand(result.editing, inputSource, *command.command);
  result.editing = applied.state;
  result.ok = applied.accepted;
  result.operationAccepted = applied.accepted;
  result.primitiveId =
      applied.edit.primitiveId.empty() ? std::string{"none"}
                                      : applied.edit.primitiveId;
  if (applied.accepted) {
    result.status = "room_editor_command_applied";
    result.reasonCode = "room_editor_command_applied";
  } else {
    result.status = applied.status;
    result.reasonCode = applied.reasonCode;
  }
  return result;
}

bool nearly(float lhs, float rhs) {
  return std::fabs(lhs - rhs) <= 0.001F;
}

bool samePoint(Vec3 lhs, Vec3 rhs) {
  return nearly(lhs.x, rhs.x) && nearly(lhs.y, rhs.y) && nearly(lhs.z, rhs.z);
}

bool wallMatchesEdge(const EditableRoomWall& wall, Vec3 startMeters, Vec3 endMeters) {
  return samePoint(wall.startMeters, startMeters) &&
         samePoint(wall.endMeters, endMeters);
}

std::optional<RoomEditCommand> buildDeleteCommand(
    const ProductRoomEditingState& editing,
    const ProductRoomEditorCursorState& cursor,
    std::string& primitiveId) {
  const EditableRoomDocument& document = editing.authoringSnapshot.document;
  const ProductRoomEditorCursorResult target =
      buildProductRoomEditorPlaceCommand(cursor, &document);
  // branch-gate: BG-1036
  if (!target.ok || !target.command.has_value()) {
    return std::nullopt;
  }
  // branch-gate: BG-1036
  switch (cursor.selectedTool) {
    case ProductRoomEditorTool::Floor: {
      const EditableRoomFloor& targetFloor = target.command->floor;
      for (const EditableRoomFloor& floor : document.floors) {
        // branch-gate: BG-1036
        if (floor.storyIndex == cursor.storyIndex &&
            samePoint(floor.centerMeters, targetFloor.centerMeters)) {
          primitiveId = floor.id;
          return deleteFloorCommand(floor.id);
        }
      }
      return std::nullopt;
    }
    case ProductRoomEditorTool::Wall: {
      const EditableRoomWall& targetWall = target.command->wall;
      for (const EditableRoomWall& wall : document.walls) {
        // branch-gate: BG-1036
        if (wall.storyIndex == cursor.storyIndex &&
            wallMatchesEdge(wall,
                            targetWall.startMeters,
                            targetWall.endMeters)) {
          primitiveId = wall.id;
          return deleteWallCommand(wall.id);
        }
      }
      return std::nullopt;
    }
    case ProductRoomEditorTool::Object: {
      const EditableRoomObject& targetObject = target.command->object;
      for (const EditableRoomObject& object : document.objects) {
        // branch-gate: BG-1036
        if (object.storyIndex == cursor.storyIndex &&
            samePoint(object.positionMeters, targetObject.positionMeters)) {
          primitiveId = object.id;
          return deleteObjectCommand(object.id);
        }
      }
      return std::nullopt;
    }
  }
  return std::nullopt;
}

ProductRoomEditorActionResult operationResult(
    const ProductRoomEditingState& editing,
    ProductRoomEditorCursorState cursor,
    std::string operation,
    ProductRoomEditingOperationResult applied,
    std::string_view acceptedStatus) {
  ProductRoomEditorActionResult result =
      baseResult(editing, cursor, std::move(operation));
  result.handled = true;
  result.editing = applied.state;
  result.ok = applied.accepted;
  result.operationAccepted = applied.accepted;
  // branch-gate: BG-1036
  result.primitiveId =
      applied.edit.primitiveId.empty() ? std::string{"none"}
                                      : applied.edit.primitiveId;
  // branch-gate: BG-1036
  if (applied.accepted) {
    result.status = std::string(acceptedStatus);
    result.reasonCode = result.status;
  } else {
    result.status = applied.status;
    result.reasonCode = applied.reasonCode;
  }
  return result;
}

ProductRoomEditorActionResult deleteResult(
    const ProductRoomEditingState& editing,
    ProductRoomEditorCursorState cursor,
    std::string operation,
    ProductRoomAuthoringInputSource inputSource) {
  ProductRoomEditorActionResult result =
      baseResult(editing, cursor, std::move(operation));
  result.handled = true;

  std::string primitiveId = "none";
  std::optional<RoomEditCommand> command =
      buildDeleteCommand(editing, cursor, primitiveId);
  // branch-gate: BG-1036
  if (!command.has_value()) {
    result.status = "room_editor_delete_target_not_found";
    result.reasonCode = result.status;
    return result;
  }

  ProductRoomEditingOperationResult applied =
      applyProductRoomEditingCommand(result.editing, inputSource, *command);
  result = operationResult(editing,
                           cursor,
                           result.operation,
                           std::move(applied),
                           "room_editor_delete_applied");
  // branch-gate: BG-1036
  if (result.primitiveId == "none") {
    result.primitiveId = primitiveId;
  }
  return result;
}

ProductRoomEditorActionResult undoResult(
    const ProductRoomEditingState& editing,
    ProductRoomEditorCursorState cursor,
    std::string operation,
    ProductRoomAuthoringInputSource inputSource) {
  ProductRoomEditingState next = editing;
  ProductRoomEditingOperationResult applied =
      undoProductRoomEditing(next, inputSource);
  return operationResult(editing,
                         cursor,
                         std::move(operation),
                         std::move(applied),
                         "room_editor_undo_applied");
}

ProductRoomEditorActionResult redoResult(
    const ProductRoomEditingState& editing,
    ProductRoomEditorCursorState cursor,
    std::string operation,
    ProductRoomAuthoringInputSource inputSource) {
  ProductRoomEditingState next = editing;
  ProductRoomEditingOperationResult applied =
      redoProductRoomEditing(next, inputSource);
  return operationResult(editing,
                         cursor,
                         std::move(operation),
                         std::move(applied),
                         "room_editor_redo_applied");
}

bool buttonIntent(const ActionStateEntry& action) {
  return action.down || action.pressed;
}

}  // namespace

ProductRoomEditorActionResult applyProductRoomEditorAction(
    const ProductRoomEditingState& editing,
    ProductRoomEditorCursorState cursor,
    const ActionStateEntry& action,
    ProductRoomAuthoringInputSource inputSource) {
  const std::string operation(inputActionName(action.action));
  if (inputActionGroup(action.action) != InputActionGroup::Editor) {
    ProductRoomEditorActionResult result =
        ignoredResult(editing, cursor, operation);
    result.status = "room_editor_action_not_supported";
    result.reasonCode = result.status;
    return result;
  }

  if (!editing.ready) {
    return notReadyResult(editing, cursor, operation);
  }

  switch (action.action) {
    case InputAction::EditorNudgeX:
      if (action.value > 0.0F) {
        return cursorResult(editing,
                            moveProductRoomEditorCursor(
                                cursor, ProductRoomEditorDirection::Right),
                            operation);
      }
      if (action.value < 0.0F) {
        return cursorResult(editing,
                            moveProductRoomEditorCursor(
                                cursor, ProductRoomEditorDirection::Left),
                            operation);
      }
      return ignoredResult(editing, cursor, operation);
    case InputAction::EditorNudgeZ:
      if (action.value > 0.0F) {
        return cursorResult(editing,
                            moveProductRoomEditorCursor(
                                cursor, ProductRoomEditorDirection::Down),
                            operation);
      }
      if (action.value < 0.0F) {
        return cursorResult(editing,
                            moveProductRoomEditorCursor(
                                cursor, ProductRoomEditorDirection::Up),
                            operation);
      }
      return ignoredResult(editing, cursor, operation);
    case InputAction::EditorNextTool:
    case InputAction::EditorPreviousTool:
      if (!buttonIntent(action)) {
        return ignoredResult(editing, cursor, operation);
      }
      return cursorResult(editing, cycleProductRoomEditorTool(cursor), operation);
    case InputAction::EditorSelectFloorTool:
      // branch-gate: BG-1040
      if (!buttonIntent(action)) {
        return ignoredResult(editing, cursor, operation);
      }
      return cursorResult(editing,
                          setProductRoomEditorTool(
                          cursor, ProductRoomEditorTool::Floor),
                          operation);
    case InputAction::EditorSelectWallTool:
      // branch-gate: BG-1040
      if (!buttonIntent(action)) {
        return ignoredResult(editing, cursor, operation);
      }
      return cursorResult(editing,
                          setProductRoomEditorTool(
                              cursor, ProductRoomEditorTool::Wall),
                          operation);
    case InputAction::EditorRotateWallDirection:
      // branch-gate: BG-1041
      if (!buttonIntent(action)) {
        return ignoredResult(editing, cursor, operation);
      }
      return cursorResult(
          editing, rotateProductRoomEditorWallDirectionClockwise(cursor),
          operation);
    case InputAction::EditorPlace:
    case InputAction::EditorApply:
      if (!buttonIntent(action)) {
        return ignoredResult(editing, cursor, operation);
      }
      return placeResult(editing, cursor, action, inputSource);
    case InputAction::EditorDelete:
      // branch-gate: BG-1036
      if (!buttonIntent(action)) {
        return ignoredResult(editing, cursor, operation);
      }
      return deleteResult(editing, cursor, operation, inputSource);
    case InputAction::EditorUndo:
      // branch-gate: BG-1036
      if (!buttonIntent(action)) {
        return ignoredResult(editing, cursor, operation);
      }
      return undoResult(editing, cursor, operation, inputSource);
    case InputAction::EditorRedo:
      // branch-gate: BG-1036
      if (!buttonIntent(action)) {
        return ignoredResult(editing, cursor, operation);
      }
      return redoResult(editing, cursor, operation, inputSource);
    case InputAction::EditorToggle:
    case InputAction::EditorSelect:
    case InputAction::EditorPreviewPlacement:
    case InputAction::EditorConfirmPreview:
    case InputAction::EditorCancelPreview:
    case InputAction::EditorResizeX:
    case InputAction::EditorResizeZ: {
      ProductRoomEditorActionResult result =
          ignoredResult(editing, cursor, operation);
      result.status = "room_editor_action_not_supported";
      result.reasonCode = result.status;
      return result;
    }
    case InputAction::None:
    case InputAction::MenuUp:
    case InputAction::MenuDown:
    case InputAction::MenuLeft:
    case InputAction::MenuRight:
    case InputAction::MenuConfirm:
    case InputAction::MenuBack:
    case InputAction::MenuNextTab:
    case InputAction::MenuPreviousTab:
    case InputAction::SystemPause:
    case InputAction::SystemDevTools:
    case InputAction::SystemHardQuit:
    case InputAction::PlayerMoveX:
    case InputAction::PlayerMoveY:
    case InputAction::PlayerLookX:
    case InputAction::PlayerLookY:
    case InputAction::PlayerJump:
    case InputAction::PlayerCrouch:
    case InputAction::PlayerDash:
    case InputAction::PlayerInteract:
    case InputAction::PlayerAttack:
    case InputAction::PlayerCast:
    case InputAction::PlayerRetryOrReset:
    case InputAction::DevToggle:
    case InputAction::DevDebugOverlay:
    case InputAction::DevCollisionOverlay:
    case InputAction::DevReceiptDump:
      break;
  }
  return ignoredResult(editing, cursor, operation);
}

ProductRoomEditorActionResult applyProductRoomEditorActions(
    const ProductRoomEditingState& editing,
    ProductRoomEditorCursorState cursor,
    const ActionState& actions,
    ProductRoomAuthoringInputSource inputSource) {
  ProductRoomEditingState currentEditing = editing;
  ProductRoomEditorCursorState currentCursor = cursor;
  ProductRoomEditorActionResult last =
      ignoredResult(currentEditing, currentCursor, "none");
  for (const ActionStateEntry& entry : actions.entries) {
    ProductRoomEditorActionResult applied =
        applyProductRoomEditorAction(currentEditing, currentCursor, entry,
                                     inputSource);
    currentEditing = applied.editing;
    currentCursor = applied.cursor;
    if (applied.handled || applied.status != "room_editor_action_ignored") {
      last = applied;
    }
  }
  last.editing = currentEditing;
  last.cursor = currentCursor;
  return last;
}

ProductRoomEditorActionResult applyProductRoomEditorMousePick(
    const ProductRoomEditingState& editing,
    const ProductRoomEditorMousePickRequest& request,
    std::string operation) {
  ProductRoomEditorMousePickRequest pickRequest = request;
  pickRequest.roomEditingReady = editing.ready && request.roomEditingReady;
  const ProductRoomEditorMousePickResult pick =
      pickProductRoomEditorCursorFromScreen(pickRequest);

  ProductRoomEditorActionResult result =
      baseResult(editing, pick.cursor, std::move(operation));
  result.handled = true;
  result.ok = pick.ok;
  result.operationAccepted = pick.ok;
  result.status = pick.status;
  result.reasonCode = pick.reasonCode;
  return result;
}

}  // namespace iggy3d
