#pragma once

#include <string>

#include "app/input/InputAction.hpp"

namespace iggy3d {

struct ActionState;

struct GamepadMenuState {
  bool initialized = false;
  bool gamepadAvailable = false;
  bool upWasDown = false;
  bool downWasDown = false;
  bool leftWasDown = false;
  bool rightWasDown = false;
  bool confirmWasDown = false;
  bool backWasDown = false;
  bool optionsWasDown = false;
  bool gameplayInteractWasDown = false;
  bool rightTriggerWasDown = false;
  bool editorUpWasDown = false;
  bool editorDownWasDown = false;
  bool editorLeftWasDown = false;
  bool editorRightWasDown = false;
  bool editorPlaceWasDown = false;
  bool editorNextToolWasDown = false;
  bool editorPreviousToolWasDown = false;
  std::string gamepadName = "unavailable";
  void* nativeGamepad = nullptr;
};

struct GamepadRoomEditorInputSample {
  bool upDown = false;
  bool downDown = false;
  bool leftDown = false;
  bool rightDown = false;
  bool placeDown = false;
  bool nextToolDown = false;
  bool previousToolDown = false;
};

struct GamepadControllerModeChordSample {
  bool leftTriggerDown = false;
  bool rightTriggerDown = false;
  bool leftStickPressDown = false;
  bool rightStickPressDown = false;
};

struct GamepadControllerActionSample {
  float leftStickX = 0.0F;
  float leftStickY = 0.0F;
  float rightStickX = 0.0F;
  float rightStickY = 0.0F;
  bool dpadUpDown = false;
  bool dpadDownDown = false;
  bool dpadLeftDown = false;
  bool dpadRightDown = false;
  bool southButtonDown = false;
  bool eastButtonDown = false;
  bool westButtonDown = false;
  bool northButtonDown = false;
  bool leftShoulderDown = false;
  bool rightShoulderDown = false;
  bool leftTriggerDown = false;
  bool rightTriggerDown = false;
  bool leftStickPressDown = false;
  bool rightStickPressDown = false;
};

void initializeGamepadMenuState(GamepadMenuState& state);
void shutdownGamepadMenuState(GamepadMenuState& state);
InputAction pollGamepadMenuAction(GamepadMenuState& state);
GamepadControllerModeChordSample pollGamepadControllerModeChordSample(
    GamepadMenuState& state);
GamepadControllerActionSample pollGamepadControllerActionSample(
    GamepadMenuState& state);
void pollGamepadGameplayActions(GamepadMenuState& state, ActionState& actions);
void recordGamepadRoomEditorActions(GamepadMenuState& state,
                                    const GamepadRoomEditorInputSample& sample,
                                    ActionState& actions);
void pollGamepadRoomEditorActions(GamepadMenuState& state, ActionState& actions);

}  // namespace iggy3d
