#include "app/iggy3d/product/AutomationGameplay.hpp"

#include <array>
#include <algorithm>

#include "app/frontend/FrontendState.hpp"
#include "app/iggy3d/ProductActiveRoomCollision.hpp"
#include "app/iggy3d/ProductGameplayController.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/input/ActionState.hpp"
#include "app/input/InputRouter.hpp"
#include "runtime/session/Session.hpp"

namespace iggy3d {

namespace {

struct GameplayAutomationRow {
  ProductAutomationCommandId commandId;
  InputAction action;
};

ProductAutomationExecutionResult unhandledGameplayAutomation() {
  return {};
}

ProductAutomationExecutionResult failGameplayAutomation() {
  return {true, false};
}

ProductAutomationExecutionResult passGameplayAutomation(bool accepted) {
  return {true, accepted};
}

bool gameplayAutomationReady(ProductAutomationGameplayContext& context) {
  // branch-gate: BG-1010
  if (context.frontend.screen == FrontendScreen::Gameplay &&
      context.window.gameplayActive &&
      context.activeSession != nullptr) {
    return true;
  }
  context.window.automationControlStatus = "owner_unavailable";
  return false;
}

bool applyGameplayActionState(ProductAutomationGameplayContext& context,
                              InputAction action,
                              const ActionState& actions) {
  InputRoutingContext routingContext;
  routingContext.owners.gameplay = true;
  const InputRoutingResult routed = routeInputAction(routingContext, action);
  context.window.inputOwner = routed.owner;
  context.window.lastInputAction = routed.action;
  context.window.lastInputAccepted = routed.accepted;
  context.window.gameplayInputSuppressed = routed.gameplaySuppressed;
  // branch-gate: BG-1010
  if (!routed.accepted) {
    return false;
  }

  applyProductGameplayActions(
      *context.activeSession, actions, context.window, "automation",
      productActiveRoomCollisionSurfaces(context.window.activeRoomCollision));
  return context.window.gameplayCommandSubmitted &&
         context.window.gameplayCommandAccepted &&
         context.window.gameplayTickAdvanced;
}

bool applyAutomationGameplayAxis(ProductAutomationGameplayContext& context,
                                 InputAction action,
                                 float value) {
  // branch-gate: BG-1010
  if (!gameplayAutomationReady(context)) {
    return false;
  }

  ActionState actions;
  recordAction(actions, action, true, false, false, value);
  return applyGameplayActionState(context, action, actions);
}

bool applyAutomationGameplayButton(ProductAutomationGameplayContext& context,
                                   InputAction action) {
  // branch-gate: BG-1010
  if (!gameplayAutomationReady(context)) {
    return false;
  }

  ActionState actions;
  recordAction(actions, action, true, true, false, 1.0F);
  return applyGameplayActionState(context, action, actions);
}

}  // namespace

ProductAutomationExecutionResult applyProductGameplayAutomationCommand(
    const ProductAutomationCommand& command,
    const ProductAutomationCommandDispatchSpec& automationSpec,
    ProductAutomationGameplayContext& context) {
  const std::string_view value{command.value};
  static constexpr std::array axisRows{
      GameplayAutomationRow{ProductAutomationCommandId::GameplayMoveX,
                            InputAction::PlayerMoveX},
      GameplayAutomationRow{ProductAutomationCommandId::GameplayMoveY,
                            InputAction::PlayerMoveY},
  };
  const auto axis = std::find_if(
      axisRows.begin(), axisRows.end(),
      [&automationSpec](const GameplayAutomationRow& row) {
        return row.commandId == automationSpec.commandId;
      });
  // branch-gate: BG-1010
  if (axis != axisRows.end()) {
    const ProductGameplayAxisAutomationResult axisResult =
        resolveProductGameplayAxisAutomation(value);
    // branch-gate: BG-1010
    if (!axisResult.valid) {
      context.window.automationControlStatus = "invalid_value";
      return failGameplayAutomation();
    }
    const bool moved = applyAutomationGameplayAxis(context, axis->action,
                                                   axisResult.value);
    markAutomationApplied(context.window, command, inputActionName(axis->action),
                          context.currentOwner(),
                          // branch-gate: BG-1010
                          moved ? "applied" : "failed");
    return passGameplayAutomation(moved);
  }

  static constexpr std::array buttonRows{
      GameplayAutomationRow{ProductAutomationCommandId::GameplayAttack,
                            InputAction::PlayerAttack},
      GameplayAutomationRow{ProductAutomationCommandId::GameplayInteract,
                            InputAction::PlayerInteract},
  };
  const auto button = std::find_if(
      buttonRows.begin(), buttonRows.end(),
      [&automationSpec](const GameplayAutomationRow& row) {
        return row.commandId == automationSpec.commandId;
      });
  // branch-gate: BG-1010
  if (button != buttonRows.end()) {
    bool boolValue = false;
    // branch-gate: BG-1010
    if (!resolveProductAutomationBool(value, boolValue)) {
      context.window.automationControlStatus = "invalid_value";
      return failGameplayAutomation();
    }
    // branch-gate: BG-1010
    if (!boolValue) {
      markAutomationApplied(context.window, command, inputActionName(button->action),
                            context.currentOwner(), "ignored");
      return passGameplayAutomation(true);
    }
    const bool executed = applyAutomationGameplayButton(context, button->action);
    markAutomationApplied(context.window, command, inputActionName(button->action),
                          context.currentOwner(),
                          // branch-gate: BG-1010
                          executed ? "applied" : "failed");
    return passGameplayAutomation(executed);
  }

  return unhandledGameplayAutomation();
}

}  // namespace iggy3d
