#include "app/input/AutomationInput.hpp"

#include "app/input/InputBindings.hpp"

namespace iggy3d {

InputAction automationControlAction(std::string_view control) {
  return actionForInput(neutralInputFromName(control));
}

}  // namespace iggy3d
