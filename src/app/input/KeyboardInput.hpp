#pragma once

#include <array>

#include "app/input/InputAction.hpp"

namespace iggy3d {

struct ActionState;

struct KeyboardInputState {
  bool upWasDown = false;
  bool downWasDown = false;
  bool leftWasDown = false;
  bool rightWasDown = false;
  bool confirmWasDown = false;
  bool backWasDown = false;
  bool tabWasDown = false;
  std::array<bool, 7> asciiPaintWasDown{};
  bool interactWasDown = false;
  bool retryWasDown = false;
};

InputAction pollKeyboardMenuAction(KeyboardInputState& state);
char pollKeyboardAsciiRoomPaintGlyph(KeyboardInputState& state);
void pollKeyboardGameplayActions(KeyboardInputState& state, ActionState& actions);

}  // namespace iggy3d
