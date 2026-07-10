#include "app/iggy3d/automation/AutomationDispatch.hpp"

#include <array>
#include <algorithm>
#include <cmath>

#include "app/frontend/FrontendState.hpp"
#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollision.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.hpp"
#include "app/iggy3d/gameplay/ProductRoomStore.hpp"
#include "app/iggy3d/input/ControllerActionMap.hpp"
#include "app/iggy3d/input/ControllerActionRouting.hpp"
#include "app/iggy3d/gameplay/Controller.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"
#include "app/iggy3d/window/InputFrame.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/input/ActionState.hpp"
#include "app/input/InputRouter.hpp"
#include "runtime/world/WorldState.hpp"
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
      context.window.gameplay.gameplayActive &&
      context.activeSession != nullptr) {
    return true;
  }
  context.window.automationControl.status = "owner_unavailable";
  return false;
}

const SpatialSurfaceSet* currentAutomationCollisionSurfaces(
    ProductAutomationGameplayContext& context) {
  (void)ensureActiveRoomCollisionFresh(context.window, context.activeSession);
  return productActiveRoomCollisionSurfaces(activeRoomCollision(context.window));
}

bool applyGameplayActionState(ProductAutomationGameplayContext& context,
                              InputAction action,
                              const ActionState& actions) {
  InputRoutingContext routingContext;
  routingContext.owners.gameplay = true;
  const InputRoutingResult routed = routeInputAction(routingContext, action);
  context.window.inputDevice.lastInputAction = routed.action;
  context.window.inputDevice.lastInputAccepted = routed.accepted;
  // branch-gate: BG-1010
  if (!routed.accepted) {
    return false;
  }

  applyProductGameplayActions(
      *context.activeSession, actions, context.window, "automation",
      currentAutomationCollisionSurfaces(context));
  (void)ensureActiveRoomCollisionFresh(context.window, context.activeSession);
  return context.window.gameplay.gameplayCommand.submitted &&
         context.window.gameplay.gameplayCommand.accepted &&
         context.window.gameplay.gameplayTickAdvanced;
}

bool applyGameplayJumpActionState(ProductAutomationGameplayContext& context,
                                  const ActionState& actions) {
  InputRoutingContext routingContext;
  routingContext.owners.gameplay = true;
  const InputRoutingResult routed =
      routeInputAction(routingContext, InputAction::PlayerJump);
  context.window.inputDevice.lastInputAction = routed.action;
  context.window.inputDevice.lastInputAccepted = routed.accepted;
  // branch-gate: BG-1010
  if (!routed.accepted) {
    return false;
  }

  applyProductGameplayActions(
      *context.activeSession, actions, context.window, "automation",
      currentAutomationCollisionSurfaces(context));
  (void)ensureActiveRoomCollisionFresh(context.window, context.activeSession);
  return context.window.gameplay.gameplayJump.requested &&
         (context.window.gameplay.gameplayJump.accepted ||
          context.window.gameplay.gameplayTraversal.consumed);
}

bool parseGameplayPosition(std::string_view value, Vec3& out) {
  const std::vector<std::string_view> fields = splitProductAutomationCsv(value);
  // branch-gate: BG-1010
  if (fields.size() != 3U) {
    return false;
  }
  Vec3 parsed;
  // branch-gate: BG-1010
  if (!parseProductAutomationFloat(fields[0], parsed.x) ||
      !parseProductAutomationFloat(fields[1], parsed.y) ||
      !parseProductAutomationFloat(fields[2], parsed.z)) {
    return false;
  }
  // branch-gate: BG-1010
  if (!std::isfinite(parsed.x) || !std::isfinite(parsed.y) ||
      !std::isfinite(parsed.z)) {
    return false;
  }
  out = parsed;
  return true;
}

bool applyAutomationGameplayPlayerPosition(
    ProductAutomationGameplayContext& context,
    const Vec3& position) {
  // branch-gate: BG-1010
  if (!gameplayAutomationReady(context)) {
    return false;
  }

  SessionState& state = context.activeSession->mutableStateForOwnedSystems();
  const EntityId actor = state.players.actorForSlot(0);
  const EntityState* entity = state.world.findById(actor);
  // branch-gate: BG-1010
  if (entity == nullptr) {
    return false;
  }

  Transform3 transform = entity->transform;
  transform.position = position;
  const WorldMutationResult mutation = state.world.updateTransform(actor, transform);
  // branch-gate: BG-1010
  if (mutation.status != WorldStatus::Ok) {
    return false;
  }

  state.currentStateHash = computeStateHash(state);
  context.window.gameplay.playerPositionChanged = true;
  return true;
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

bool applyAutomationGameplayJump(ProductAutomationGameplayContext& context) {
  // branch-gate: BG-1010
  if (!gameplayAutomationReady(context)) {
    return false;
  }

  ActionState actions;
  recordAction(actions, InputAction::PlayerJump, true, true, false, 1.0F);
  return applyGameplayJumpActionState(context, actions);
}

bool parseControllerInputToken(std::string_view token,
                               GamepadControllerActionSample& out) {
  // branch-gate: BG-1062
  if (token == "release" || token == "none") {
    out = {};
    return true;
  }
  // branch-gate: BG-1062
  if (token == "mode_chord") {
    out = productControllerModeChordActionSample();
    return true;
  }
  ProductControllerControl control = ProductControllerControl::None;
  // branch-gate: BG-1062
  if (!parseProductControllerControlName(token, control) ||
      control == ProductControllerControl::None) {
    return false;
  }
  out = productControllerActionSampleForControl(control);
  return true;
}

bool applyControllerInputSequence(ProductAutomationGameplayContext& context,
                                  std::string_view value) {
  const std::vector<std::string_view> tokens = splitProductAutomationCsv(value);
  // branch-gate: BG-1062
  if (tokens.empty()) {
    context.window.automationControl.status = "invalid_value";
    return false;
  }

  ProductControllerModeChordState chordState;
  ProductControllerActionRoutingState actionState;
  bool processed = false;
  for (const std::string_view token : tokens) {
    GamepadControllerActionSample sample;
    // branch-gate: BG-1062
    if (!parseControllerInputToken(token, sample)) {
      context.window.automationControl.status = "invalid_value";
      return false;
    }
    ProductControllerSampleInputResult result =
        processProductControllerActionSample({
            context.frontend,
            context.window,
            context.activeSession,
            chordState,
            actionState,
            nullptr,
            "controller",
        },
                                             sample);
    processed = processed || result.processed;
  }
  return processed;
}

}  // namespace

ProductAutomationExecutionResult applyProductGameplayAutomationCommand(
    const ProductAutomationCommand& command,
    const ProductAutomationCommandDispatchSpec& automationSpec,
    ProductAutomationGameplayContext& context) {
  const std::string_view value{command.value};
  // branch-gate: BG-1010
  if (automationSpec.commandId ==
      ProductAutomationCommandId::GameplayPhysicsMovement) {
    bool boolValue = false;
    // branch-gate: BG-1010
    if (!resolveProductAutomationBool(value, boolValue)) {
      context.window.automationControl.status = "invalid_value";
      return failGameplayAutomation();
    }
    context.window.gameplay.physicsMovementPlanner.enabled = boolValue;
    // branch-gate: BG-1010
    if (!boolValue) {
      recordProductPhysicsMovementPlannerTickProof(context.window, false,
                                                   false, false);
    }
    markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                          context.currentOwner(), "applied");
    return passGameplayAutomation(true);
  }

  // branch-gate: BG-1010
  if (automationSpec.commandId ==
      ProductAutomationCommandId::GameplayPlayerPosition) {
    Vec3 position;
    // branch-gate: BG-1010
    if (!parseGameplayPosition(value, position)) {
      context.window.automationControl.status = "invalid_value";
      return failGameplayAutomation();
    }
    const bool positioned =
        applyAutomationGameplayPlayerPosition(context, position);
    markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                          context.currentOwner(),
                          // branch-gate: BG-1010
                          positioned ? "applied" : "failed");
    return passGameplayAutomation(positioned);
  }

  // branch-gate: BG-1062
  if (automationSpec.commandId == ProductAutomationCommandId::ControllerInput) {
    const bool processed = applyControllerInputSequence(context, value);
    markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                          context.currentOwner(),
                          // branch-gate: BG-1062
                          processed ? "applied" : "failed");
    return passGameplayAutomation(processed);
  }

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
      context.window.automationControl.status = "invalid_value";
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
      context.window.automationControl.status = "invalid_value";
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

  // branch-gate: BG-1010
  if (automationSpec.commandId == ProductAutomationCommandId::GameplayJump) {
    bool boolValue = false;
    // branch-gate: BG-1010
    if (!resolveProductAutomationBool(value, boolValue)) {
      context.window.automationControl.status = "invalid_value";
      return failGameplayAutomation();
    }
    // branch-gate: BG-1010
    if (!boolValue) {
      markAutomationApplied(context.window, command, automationSpec.canonicalKey,
                            context.currentOwner(), "ignored");
      return passGameplayAutomation(true);
    }
    const bool jumped = applyAutomationGameplayJump(context);
    markAutomationApplied(context.window, command, inputActionName(InputAction::PlayerJump),
                          context.currentOwner(),
                          // branch-gate: BG-1010
                          jumped ? "applied" : "failed");
    return passGameplayAutomation(jumped);
  }

  return unhandledGameplayAutomation();
}

}  // namespace iggy3d
