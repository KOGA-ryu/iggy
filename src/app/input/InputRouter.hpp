#pragma once

#include "app/frontend/MenuInput.hpp"
#include "app/input/InputAction.hpp"

namespace iggy3d {

struct InputRoutingContext {
  MenuOwnerState owners;
};

struct InputRoutingResult {
  MenuOwner owner = MenuOwner::None;
  InputAction action = InputAction::None;
  bool accepted = false;
  bool gameplaySuppressed = false;
};

InputRoutingResult routeInputAction(const InputRoutingContext& context, InputAction action);

}  // namespace iggy3d
