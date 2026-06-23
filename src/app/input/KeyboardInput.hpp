#pragma once

#include "app/input/InputAction.hpp"

namespace iggy3d {

struct KeyboardInputState {
  bool upWasDown = false;
  bool downWasDown = false;
  bool confirmWasDown = false;
  bool backWasDown = false;
};

InputAction pollKeyboardMenuAction(KeyboardInputState& state);

}  // namespace iggy3d
