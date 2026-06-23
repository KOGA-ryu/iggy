#pragma once

#include <string>

#include "app/input/InputAction.hpp"

namespace iggy3d {

struct GamepadMenuState {
  bool initialized = false;
  bool gamepadAvailable = false;
  bool upWasDown = false;
  bool downWasDown = false;
  bool confirmWasDown = false;
  bool backWasDown = false;
  bool optionsWasDown = false;
  std::string gamepadName = "unavailable";
  void* nativeGamepad = nullptr;
};

void initializeGamepadMenuState(GamepadMenuState& state);
void shutdownGamepadMenuState(GamepadMenuState& state);
InputAction pollGamepadMenuAction(GamepadMenuState& state);

}  // namespace iggy3d
