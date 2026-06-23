#pragma once

#include "app/input/InputAction.hpp"

namespace iggy3d {

struct MouseInputState {
  bool leftWasDown = false;
};

struct MouseClick {
  bool clicked = false;
  float x = 0.0F;
  float y = 0.0F;
};

MouseClick pollMouseClick(MouseInputState& state);
InputAction mouseClickAction(const MouseClick& click);

}  // namespace iggy3d
