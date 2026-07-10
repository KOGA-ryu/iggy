#pragma once

#include "app/input/InputActions.hpp"

namespace iggy3d {

struct ActionState;

struct MouseInputState {
  bool leftWasDown = false;
};

struct MouseClick {
  bool clicked = false;
  float x = 0.0F;
  float y = 0.0F;
};

MouseClick pollMouseClick(MouseInputState& state);
void pollMouseGameplayActions(MouseInputState& state, ActionState& actions);
InputAction mouseClickAction(const MouseClick& click);

}  // namespace iggy3d
