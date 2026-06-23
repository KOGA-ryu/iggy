#pragma once

#include <string_view>
#include <vector>

#include "app/input/InputAction.hpp"
#include "app/input/InputDeviceEvent.hpp"

namespace iggy3d {

enum class InputDeviceKind {
  Keyboard,
  Mouse,
  Gamepad,
  Automation,
};

struct InputBinding {
  NeutralInput input = NeutralInput::None;
  InputAction action = InputAction::None;
  float scale = 1.0F;
};

std::string_view inputDeviceKindName(InputDeviceKind device);
const std::vector<InputBinding>& defaultInputBindings();
InputAction actionForInput(NeutralInput input);
InputAction actionForDeviceEvent(const InputDeviceEvent& event);
NeutralInput neutralInputFromName(std::string_view control);

}  // namespace iggy3d
