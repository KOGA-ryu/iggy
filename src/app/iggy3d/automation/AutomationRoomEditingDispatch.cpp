#include "app/iggy3d/automation/AutomationRoomEditing.hpp"

#include <string>
#include <string_view>
#include <vector>

#include "app/frontend/FrontendState.hpp"
#include "app/iggy3d/gameplay/ActiveRoomState.hpp"
#include "app/iggy3d/gameplay/ProductRoomStore.hpp"
#include "app/iggy3d/ascii_room/Preview.hpp"
#include "app/iggy3d/room_editor/AuthoringController.hpp"
#include "app/iggy3d/room_editor/ActionController.hpp"
#include "app/iggy3d/room_editor/Cursor.hpp"
#include "app/iggy3d/room_editor/Preview.hpp"
#include "app/iggy3d/room_editor/EditingState.hpp"
#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/input/ActionState.hpp"

namespace iggy3d {

ProductRoomEditorCursorResult applyProductRoomEditorMoveAutomation(
    ProductRoomEditorCursorState state,
    ProductRoomEditorDirection direction) {
  return moveProductRoomEditorCursor(state, direction);
}

ProductRoomEditorCursorResult applyProductRoomEditorToolAutomation(
    ProductRoomEditorCursorState state,
    ProductRoomEditorTool tool) {
  return setProductRoomEditorTool(state, tool);
}

ProductRoomEditorCursorResult applyProductRoomEditorCycleToolAutomation(
    ProductRoomEditorCursorState state) {
  return cycleProductRoomEditorTool(state);
}

ProductRoomEditorCursorResult applyProductRoomEditorWallDirectionAutomation(
    ProductRoomEditorCursorState state,
    ProductRoomEditorDirection direction) {
  return setProductRoomEditorWallDirection(state, direction);
}

ProductRoomEditorActionResult applyProductRoomEditorPlaceAutomation(
    const ProductRoomEditingState& editing,
    ProductRoomEditorCursorState cursor,
    ProductRoomAuthoringInputSource inputSource) {
  return applyProductRoomAuthoringCursorPlace({editing, cursor, inputSource});
}

ProductRoomEditorActionResult applyProductEditorInputAutomation(
    const ProductRoomEditingState& editing,
    ProductRoomEditorCursorState cursor,
    const ActionState& actions,
    ProductRoomAuthoringInputSource inputSource) {
  return applyProductRoomEditorActions(editing, cursor, actions, inputSource);
}

// branch-gate-relocation: BG-1234 from=src/app/iggy3d/automation/AutomationRoomEditing.cpp

namespace {


ProductAutomationExecutionResult unhandledRoomEditingAutomation() {
  return {};
}

ProductAutomationExecutionResult failRoomEditingAutomation() {
  return {true, false};
}

ProductAutomationExecutionResult passRoomEditingAutomation(bool accepted) {
  return {true, accepted};
}

ProductAutomationExecutionResult failRoomEditorPreviewAutomation(
    const ProductAutomationCommand& command,
    const ProductAutomationCommandDispatchSpec& automationSpec,
    ProductAutomationRoomEditingContext& context,
    std::string_view reasonCode) {
  context.window.automationControl.status = "command_failed";
  markProductRoomEditorPreviewCleared(context.window, reasonCode);
  markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                        context.currentOwner(), "failed");
  return failRoomEditingAutomation();
}

struct RoomEditorMousePickValue {
  bool valid = false;
  float screenX = 0.0F;
  float screenY = 0.0F;
};

RoomEditorMousePickValue parseRoomEditorMousePickValue(std::string_view value) {
  const std::vector<std::string_view> parts = splitProductAutomationCsv(value);
  RoomEditorMousePickValue parsed;
  // branch-gate: BG-1044
  if (parts.size() != 2U) {
    return parsed;
  }
  // branch-gate: BG-1044
  if (!parseProductAutomationFloat(parts[0], parsed.screenX) ||
      !parseProductAutomationFloat(parts[1], parsed.screenY)) {
    return parsed;
  }
  parsed.valid = true;
  return parsed;
}

bool roomEditorReady(ProductAutomationRoomEditingContext& context,
                     const ProductAutomationCommand& command,
                     std::string_view operation) {
  // branch-gate: BG-1006
  if (context.window.creativeAuthoring.roomEditing.ready) {
    return true;
  }
  rejectProductRoomEditorNotReady(context.window, operation);
  markAutomationApplied(context.window, command, operation, context.currentOwner(),
                        "failed");
  return false;
}

ProductAutomationExecutionResult applyRoomEditingOperationResult(
    const ProductAutomationCommand& command,
    const ProductAutomationCommandDispatchSpec& automationSpec,
    ProductAutomationRoomEditingContext& context,
    ProductRoomEditingOperationResult& result) {
  recordProductRoomEditingOperation(context.window, automationSpec.canonicalKey,
                                    result);
  // branch-gate: BG-1006
  markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                        context.currentOwner(),
                        result.accepted ? "applied" : "failed");
  return passRoomEditingAutomation(result.accepted);
}

ProductAutomationExecutionResult applyRoomEditorCursorResult(
    const ProductAutomationCommand& command,
    const ProductAutomationCommandDispatchSpec& automationSpec,
    ProductAutomationRoomEditingContext& context,
    const ProductRoomEditorCursorResult& result) {
  recordProductRoomEditorCursorResult(context.window, automationSpec.canonicalKey,
                                      result);
  // branch-gate: BG-1006
  if (!result.ok) {
    context.window.automationControl.status = "command_failed";
  }
  // branch-gate: BG-1006
  markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                        context.currentOwner(),
                        result.ok ? "applied" : "failed");
  return passRoomEditingAutomation(result.ok);
}
}  // namespace

ProductAutomationExecutionResult applyProductRoomEditingAutomationCommand(
    const ProductAutomationCommand& command,
    const ProductAutomationCommandDispatchSpec& automationSpec,
    ProductAutomationRoomEditingContext& context) {
  const std::string_view value{command.value};
  bool boolValue = false;

  // branch-gate: BG-1006
  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditStart) {
    // branch-gate: BG-1006
    if (!resolveProductAutomationBool(value, boolValue)) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    // branch-gate: BG-1006
    if (!boolValue) {
      markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                            context.currentOwner(), "ignored");
      return passRoomEditingAutomation(true);
    }

    const ProductAsciiRoomAuthoringRequest request =
        productAsciiRoomAuthoringRequestFromDraft(context.window);
    const ProductRoomEditingStartResult started =
        startProductRoomEditAutomationFromAsciiDraft({request});
    recordProductRoomEditingStart(context.window, started);
    // branch-gate: BG-1006
    markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                          context.currentOwner(),
                          started.ok ? "applied" : "failed");
    return passRoomEditingAutomation(started.ok);
  }

  // branch-gate: BG-1006
  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditStartActive) {
    // branch-gate: BG-1006
    if (!resolveProductAutomationBool(value, boolValue)) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    // branch-gate: BG-1006
    if (!boolValue) {
      markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                            context.currentOwner(), "ignored");
      return passRoomEditingAutomation(true);
    }

    const ProductRoomEditingStartResult started =
        startProductRoomEditAutomationFromActiveRoom({activeRoom(context.window)});
    recordProductRoomEditingStart(context.window, started, automationSpec.canonicalKey);
    // branch-gate: BG-1006
    markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                          context.currentOwner(),
                          started.ok ? "applied" : "failed");
    return passRoomEditingAutomation(started.ok);
  }

  // branch-gate: BG-1006
  if (command.key == "editor.input") {
    const std::vector<std::string_view> editorInputs =
        splitProductAutomationCsv(value);
    // branch-gate: BG-1006
    if (editorInputs.empty()) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }

    InputAction lastAction = InputAction::None;
    MenuOwner lastOwner = context.currentOwner();
    bool anyApplied = false;
    for (const std::string_view token : editorInputs) {
      InputAction editorAction = InputAction::None;
      float actionValue = 1.0F;
      // branch-gate: BG-1006
      if (!parseProductRoomEditorInputAction(token, editorAction, actionValue)) {
        context.window.automationControl.status = "invalid_value";
        return failRoomEditingAutomation();
      }
      lastAction = editorAction;
      // branch-gate: BG-1006
      if (context.frontend.screen != FrontendScreen::Gameplay ||
          !context.window.gameplay.gameplayActive ||
          !context.activeSessionAvailable ||
          !context.window.creativeAuthoring.roomEditing.ready) {
        rejectProductRoomEditorNotReady(context.window, inputActionName(editorAction));
        markAutomationApplied(context.window, command, inputActionName(editorAction),
                              context.currentOwner(), "failed");
        return failRoomEditingAutomation();
      }

      const InputRoutingResult routed = context.routeEditorInput(editorAction);
      lastOwner = routed.owner;
      context.window.inputDevice.lastInputAction = routed.action;
      context.window.inputDevice.lastInputAccepted = routed.accepted;
      // branch-gate: BG-1006
      if (!routed.accepted || routed.owner != MenuOwner::Editor) {
        context.window.automationControl.status = "owner_unavailable";
        markAutomationApplied(context.window, command, inputActionName(editorAction),
                              routed.owner, "failed");
        return failRoomEditingAutomation();
      }

      // branch-gate: BG-1054
      if (isProductRoomEditorPreviewInputAction(editorAction)) {
        const ProductRoomEditorPreviewInputResult previewResult =
            applyProductRoomEditorPreviewInputAction(context.window, editorAction);
        // branch-gate: BG-1054
        if (!previewResult.ok) {
          context.window.automationControl.status = "command_failed";
          markAutomationApplied(context.window, command, inputActionName(editorAction),
                                routed.owner, "failed");
          return failRoomEditingAutomation();
        }
        anyApplied = true;
        continue;
      }

      ActionState actions;
      recordAction(actions, editorAction, true, true, false, actionValue);
      const ProductRoomEditorActionResult result =
          applyProductEditorInputAutomation(
              context.window.creativeAuthoring.roomEditing, context.window.creativeAuthoring.roomEditorCursor, actions,
              ProductRoomAuthoringInputSource::Hotkey);
      recordProductRoomEditorActionResult(context.window, result);
      // branch-gate: BG-1006
      if (!result.ok) {
        context.window.automationControl.status = "command_failed";
        markAutomationApplied(context.window, command, inputActionName(editorAction),
                              routed.owner, "failed");
        return failRoomEditingAutomation();
      }
      anyApplied = true;
    }

    // branch-gate: BG-1006
    if (!anyApplied) {
      context.window.automationControl.status = "command_failed";
      markAutomationApplied(context.window, command, "editor.input", lastOwner,
                            "failed");
      return failRoomEditingAutomation();
    }
    markAutomationApplied(context.window, command, inputActionName(lastAction),
                          lastOwner, "applied");
    return passRoomEditingAutomation(true);
  }

  // branch-gate: BG-1006
  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditorMove) {
    ProductRoomEditorDirection direction = ProductRoomEditorDirection::Up;
    // branch-gate: BG-1006
    if (!parseProductRoomEditorDirection(value, direction)) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    // branch-gate: BG-1006
    if (!roomEditorReady(context, command, automationSpec.canonicalKey)) {
      return failRoomEditingAutomation();
    }
    return applyRoomEditorCursorResult(
        command, automationSpec, context,
        applyProductRoomEditorMoveAutomation(context.window.creativeAuthoring.roomEditorCursor,
                                             direction));
  }

  // branch-gate: BG-1006
  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditorTool) {
    ProductRoomEditorTool tool = ProductRoomEditorTool::Floor;
    // branch-gate: BG-1006
    if (!parseProductRoomEditorTool(value, tool)) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    // branch-gate: BG-1006
    if (!roomEditorReady(context, command, automationSpec.canonicalKey)) {
      return failRoomEditingAutomation();
    }
    return applyRoomEditorCursorResult(
        command, automationSpec, context,
        applyProductRoomEditorToolAutomation(context.window.creativeAuthoring.roomEditorCursor, tool));
  }

  // branch-gate: BG-1006
  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditorCycleTool) {
    // branch-gate: BG-1006
    if (!resolveProductAutomationBool(value, boolValue)) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    // branch-gate: BG-1006
    if (!boolValue) {
      markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                            context.currentOwner(), "ignored");
      return passRoomEditingAutomation(true);
    }
    // branch-gate: BG-1006
    if (!roomEditorReady(context, command, automationSpec.canonicalKey)) {
      return failRoomEditingAutomation();
    }
    return applyRoomEditorCursorResult(
        command, automationSpec, context,
        applyProductRoomEditorCycleToolAutomation(context.window.creativeAuthoring.roomEditorCursor));
  }

  // branch-gate: BG-1006
  if (automationSpec.commandId ==
      ProductAutomationCommandId::RoomEditorWallDirection) {
    ProductRoomEditorDirection direction = ProductRoomEditorDirection::Up;
    // branch-gate: BG-1006
    if (!parseProductRoomEditorDirection(value, direction)) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    // branch-gate: BG-1006
    if (!roomEditorReady(context, command, automationSpec.canonicalKey)) {
      return failRoomEditingAutomation();
    }
    return applyRoomEditorCursorResult(
        command, automationSpec, context,
        applyProductRoomEditorWallDirectionAutomation(
            context.window.creativeAuthoring.roomEditorCursor, direction));
  }

  // branch-gate: BG-1044
  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditorMousePick) {
    const RoomEditorMousePickValue pick = parseRoomEditorMousePickValue(value);
    // branch-gate: BG-1044
    if (!pick.valid) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    // branch-gate: BG-1044
    if (!roomEditorReady(context, command, automationSpec.canonicalKey)) {
      return failRoomEditingAutomation();
    }

    const ProductRoomEditorActionResult result =
        applyProductRoomEditorMousePickAutomation(
            context.window.creativeAuthoring.roomEditing,
            context.window.creativeAuthoring.roomEditorCursor,
            pick.screenX,
            pick.screenY,
            productRoomEditorMousePickViewportConfig(context.window),
            productRoomEditorMousePickAnchor(context.activeSession));
    recordProductRoomEditorActionResult(context.window, result,
                                        automationSpec.canonicalKey);
    // branch-gate: BG-1044
    if (!result.ok) {
      context.window.automationControl.status = "command_failed";
    }
    // branch-gate: BG-1044
    markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                          context.currentOwner(),
                          result.ok ? "applied" : "failed");
    return passRoomEditingAutomation(result.ok);
  }

  // branch-gate: BG-1049
  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditorPreview) {
    // branch-gate: BG-1049
    if (!resolveProductAutomationBool(value, boolValue)) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    // branch-gate: BG-1049
    if (!boolValue) {
      markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                            context.currentOwner(), "ignored");
      return passRoomEditingAutomation(true);
    }
    // branch-gate: BG-1049
    if (!roomEditorReady(context, command, automationSpec.canonicalKey)) {
      return failRoomEditingAutomation();
    }

    const ProductRoomEditorPlacementPreviewResult result =
        buildProductRoomEditorPreviewAutomation(context.window.creativeAuthoring.roomEditing,
                                                context.window.creativeAuthoring.roomEditorCursor);
    recordProductRoomEditorPreviewResult(context.window, result);
    // branch-gate: BG-1049
    if (!result.ok) {
      context.window.automationControl.status = "command_failed";
    }
    // branch-gate: BG-1049
    markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                          context.currentOwner(),
                          result.ok ? "applied" : "failed");
    return passRoomEditingAutomation(result.ok);
  }

  // branch-gate: BG-1051
  if (automationSpec.commandId ==
      ProductAutomationCommandId::RoomEditorPreviewConfirm) {
    // branch-gate: BG-1051
    if (!resolveProductAutomationBool(value, boolValue)) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    // branch-gate: BG-1051
    if (!boolValue) {
      markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                            context.currentOwner(), "ignored");
      return passRoomEditingAutomation(true);
    }
    // branch-gate: BG-1051
    if (!roomEditorReady(context, command, automationSpec.canonicalKey)) {
      return failRoomEditingAutomation();
    }
    // branch-gate: BG-1051
    if (!context.window.creativeAuthoring.roomEditorPreview.active ||
        !context.window.creativeAuthoring.roomEditorPlacementPreview.ok ||
        !context.window.creativeAuthoring.roomEditorPlacementPreview.candidateCommandReady) {
      return failRoomEditorPreviewAutomation(
          command, automationSpec, context, "room_editor_preview_confirm_missing");
    }

    ProductRoomEditingOperationResult result =
        confirmProductRoomEditorPreviewAutomation(
            context.window.creativeAuthoring.roomEditing, context.window.creativeAuthoring.roomEditorPlacementPreview);
    return applyRoomEditingOperationResult(command, automationSpec, context, result);
  }

  // branch-gate: BG-1051
  if (automationSpec.commandId ==
      ProductAutomationCommandId::RoomEditorPreviewCancel) {
    // branch-gate: BG-1051
    if (!resolveProductAutomationBool(value, boolValue)) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    // branch-gate: BG-1051
    if (!boolValue) {
      markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                            context.currentOwner(), "ignored");
      return passRoomEditingAutomation(true);
    }
    // branch-gate: BG-1051
    if (!roomEditorReady(context, command, automationSpec.canonicalKey)) {
      return failRoomEditingAutomation();
    }

    markProductRoomEditorPreviewCleared(context.window,
                                        "room_editor_preview_cancelled");
    markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                          context.currentOwner(), "applied");
    return passRoomEditingAutomation(true);
  }

  // branch-gate: BG-1006
  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditorPlace) {
    // branch-gate: BG-1006
    if (!resolveProductAutomationBool(value, boolValue)) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    // branch-gate: BG-1006
    if (!boolValue) {
      markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                            context.currentOwner(), "ignored");
      return passRoomEditingAutomation(true);
    }
    // branch-gate: BG-1006
    if (!roomEditorReady(context, command, automationSpec.canonicalKey)) {
      return failRoomEditingAutomation();
    }

    const ProductRoomEditorActionResult result =
        applyProductRoomEditorPlaceAutomation(
            context.window.creativeAuthoring.roomEditing, context.window.creativeAuthoring.roomEditorCursor,
            ProductRoomAuthoringInputSource::Hotkey);
    recordProductRoomEditorActionResult(context.window, result,
                                        automationSpec.canonicalKey);
    // branch-gate: BG-1006
    if (!result.ok) {
      context.window.automationControl.status = "command_failed";
    }
    // branch-gate: BG-1006
    markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                          context.currentOwner(),
                          result.ok ? "applied" : "failed");
    return passRoomEditingAutomation(result.ok);
  }

  // branch-gate: BG-1006
  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditAddFloor) {
    RoomEditCommand edit;
    // branch-gate: BG-1006
    if (!parseProductAutomationFloorCommand(value, edit)) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    ProductRoomEditingOperationResult result = applyProductRoomEditAutomation(
        {context.window.creativeAuthoring.roomEditing, ProductRoomAuthoringInputSource::Script, edit});
    return applyRoomEditingOperationResult(command, automationSpec, context, result);
  }

  // branch-gate: BG-1006
  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditAddWall) {
    RoomEditCommand edit;
    // branch-gate: BG-1006
    if (!parseProductAutomationWallCommand(value, edit)) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    ProductRoomEditingOperationResult result = applyProductRoomEditAutomation(
        {context.window.creativeAuthoring.roomEditing, ProductRoomAuthoringInputSource::Script, edit});
    return applyRoomEditingOperationResult(command, automationSpec, context, result);
  }

  // branch-gate: BG-1006
  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditDeleteFloor) {
    // branch-gate: BG-1006
    if (value.empty()) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    const RoomEditCommand edit = deleteFloorCommand(std::string(value));
    ProductRoomEditingOperationResult result = applyProductRoomEditAutomation(
        {context.window.creativeAuthoring.roomEditing, ProductRoomAuthoringInputSource::Script, edit});
    return applyRoomEditingOperationResult(command, automationSpec, context, result);
  }

  // branch-gate: BG-1006
  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditDeleteWall) {
    // branch-gate: BG-1006
    if (value.empty()) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    const RoomEditCommand edit = deleteWallCommand(std::string(value));
    ProductRoomEditingOperationResult result = applyProductRoomEditAutomation(
        {context.window.creativeAuthoring.roomEditing, ProductRoomAuthoringInputSource::Script, edit});
    return applyRoomEditingOperationResult(command, automationSpec, context, result);
  }

  // branch-gate: BG-1006
  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditUndo) {
    // branch-gate: BG-1006
    if (!resolveProductAutomationBool(value, boolValue)) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    // branch-gate: BG-1006
    if (!boolValue) {
      markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                            context.currentOwner(), "ignored");
      return passRoomEditingAutomation(true);
    }
    ProductRoomEditingOperationResult result =
        undoProductRoomEditAutomation(
            {context.window.creativeAuthoring.roomEditing, ProductRoomAuthoringInputSource::Script});
    return applyRoomEditingOperationResult(command, automationSpec, context, result);
  }

  // branch-gate: BG-1006
  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditRedo) {
    // branch-gate: BG-1006
    if (!resolveProductAutomationBool(value, boolValue)) {
      context.window.automationControl.status = "invalid_value";
      return failRoomEditingAutomation();
    }
    // branch-gate: BG-1006
    if (!boolValue) {
      markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                            context.currentOwner(), "ignored");
      return passRoomEditingAutomation(true);
    }
    ProductRoomEditingOperationResult result =
        redoProductRoomEditAutomation(
            {context.window.creativeAuthoring.roomEditing, ProductRoomAuthoringInputSource::Script});
    return applyRoomEditingOperationResult(command, automationSpec, context, result);
  }

  return unhandledRoomEditingAutomation();
}

}  // namespace iggy3d
