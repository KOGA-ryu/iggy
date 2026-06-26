#include "app/iggy3d/ProductRoomEditorActionController.hpp"

#include <cmath>

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
    case InputAction::EditorPlace:
    case InputAction::EditorApply:
      if (!buttonIntent(action)) {
        return ignoredResult(editing, cursor, operation);
      }
      return placeResult(editing, cursor, action, inputSource);
    case InputAction::EditorDelete:
    case InputAction::EditorUndo:
    case InputAction::EditorRedo:
    case InputAction::EditorToggle:
    case InputAction::EditorSelect:
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

}  // namespace iggy3d
