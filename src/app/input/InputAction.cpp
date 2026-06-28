#include "app/input/InputAction.hpp"
#include "app/input/InputActionRegistry.hpp"

namespace iggy3d {

std::string_view inputActionName(InputAction action) {
  return inputActionDescriptor(action).name;
}

InputActionGroup inputActionGroup(InputAction action) {
  return inputActionDescriptor(action).group;
}

std::string_view inputActionGroupName(InputActionGroup group) {
  switch (group) {
    case InputActionGroup::None:
      return "none";
    case InputActionGroup::Menu:
      return "menu";
    case InputActionGroup::System:
      return "system";
    case InputActionGroup::Player:
      return "player";
    case InputActionGroup::Editor:
      return "editor";
  }
  return "none";
}

}  // namespace iggy3d
