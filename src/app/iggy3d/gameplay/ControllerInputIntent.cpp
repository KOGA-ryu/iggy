#include "app/iggy3d/gameplay/ControllerInputIntent.hpp"

#include "app/input/ActionState.hpp"
#include "app/iggy3d/gameplay/ControllerMoveActions.hpp"

namespace iggy3d {

ProductGameplayInputIntent sampleProductGameplayInputIntent(
    const ActionState& actions) {
  ProductGameplayInputIntent intent;
  intent.moveX = actionAxisValue(actions, InputAction::PlayerMoveX);
  intent.moveY = actionAxisValue(actions, InputAction::PlayerMoveY);
  intent.sprinting = actionIsDown(actions, InputAction::PlayerSprint);
  intent.jumpPressed = actionWasPressed(actions, InputAction::PlayerJump);
  intent.jumpReleased = actionWasReleased(actions, InputAction::PlayerJump);
  intent.dashPressed = actionWasPressed(actions, InputAction::PlayerDash);
  intent.interactPressed = actionWasPressed(actions, InputAction::PlayerInteract);
  intent.attackPressed = actionWasPressed(actions, InputAction::PlayerAttack);
  intent.resetPressed =
      actionWasPressed(actions, InputAction::PlayerRetryOrReset);
  return intent;
}

bool productGameplayIntentHasMovement(
    const ProductGameplayInputIntent& intent,
    const ProductAppWindowState& window) {
  return intent.moveX != 0.0F || intent.moveY != 0.0F ||
         productHorizontalVelocityActive(window);
}

}  // namespace iggy3d
