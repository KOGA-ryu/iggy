#pragma once

#include <string_view>

#include "app/input/InputAction.hpp"

namespace iggy3d {

struct InputActionDescriptor {
  InputAction action = InputAction::None;
  std::string_view name = "none";
  InputActionGroup group = InputActionGroup::None;
  std::string_view feature = "none";
  std::string_view owner = "none";
  bool handledBeforeMenu = false;
  bool keepsGameplayActive = false;
};

const InputActionDescriptor& inputActionDescriptor(InputAction action);
std::string_view inputActionFeatureName(InputAction action);
std::string_view inputActionOwnerName(InputAction action);
bool inputActionHandledBeforeMenu(InputAction action);
bool inputActionKeepsGameplayActive(InputAction action);

}  // namespace iggy3d
