#pragma once

#include "app/input/InputAction.hpp"

namespace iggy3d {

struct ActionState;

struct KeyboardInputState {
  bool upWasDown = false;
  bool downWasDown = false;
  bool confirmWasDown = false;
  bool backWasDown = false;
  bool interactWasDown = false;
  bool retryWasDown = false;
};

InputAction pollKeyboardMenuAction(KeyboardInputState& state);
void pollKeyboardGameplayActions(KeyboardInputState& state, ActionState& actions);

}  // namespace iggy3d
