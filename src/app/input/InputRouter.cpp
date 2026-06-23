#include "app/input/InputRouter.hpp"

namespace iggy3d {

InputRoutingResult routeInputAction(const InputRoutingContext& context, InputAction action) {
  InputRoutingResult result;
  result.owner = chooseMenuOwner(context.owners);
  result.action = action;
  result.gameplaySuppressed = menuOwnerBlocksGameplay(result.owner);

  const InputActionGroup group = inputActionGroup(action);
  if (action == InputAction::None) {
    return result;
  }
  if (group == InputActionGroup::System) {
    result.accepted = true;
    return result;
  }
  if (result.owner == MenuOwner::Starter || result.owner == MenuOwner::Pause ||
      result.owner == MenuOwner::Settings || result.owner == MenuOwner::DevTools) {
    result.accepted = group == InputActionGroup::Menu;
    return result;
  }
  if (result.owner == MenuOwner::Editor) {
    result.accepted = group == InputActionGroup::Editor || group == InputActionGroup::Menu;
    return result;
  }
  if (result.owner == MenuOwner::Gameplay) {
    result.accepted = group == InputActionGroup::Player || group == InputActionGroup::System;
    return result;
  }

  return result;
}

}  // namespace iggy3d
