#include "app/input/GamepadInput.hpp"

#include "app/input/ActionState.hpp"
#include "app/input/InputBindings.hpp"

#if defined(IGGY3D_HAS_SDL3)
#include <SDL3/SDL.h>
#include <SDL3/SDL_gamepad.h>
#endif

namespace iggy3d {
namespace {

#if defined(IGGY3D_HAS_SDL3)
SDL_Gamepad* nativeGamepad(GamepadMenuState& state) {
  return static_cast<SDL_Gamepad*>(state.nativeGamepad);
}

bool gamepadButtonDown(GamepadMenuState& state, SDL_GamepadButton button) {
  SDL_Gamepad* gamepad = nativeGamepad(state);
  return gamepad != nullptr && SDL_GetGamepadButton(gamepad, button);
}

float normalizedAxis(GamepadMenuState& state, SDL_GamepadAxis axis) {
  SDL_Gamepad* gamepad = nativeGamepad(state);
  if (gamepad == nullptr) {
    return 0.0F;
  }
  constexpr float kScale = 32767.0F;
  float value = static_cast<float>(SDL_GetGamepadAxis(gamepad, axis)) / kScale;
  if (value > -0.18F && value < 0.18F) {
    return 0.0F;
  }
  if (value < -1.0F) {
    return -1.0F;
  }
  if (value > 1.0F) {
    return 1.0F;
  }
  return value;
}

bool triggerDown(GamepadMenuState& state, SDL_GamepadAxis axis) {
  return normalizedAxis(state, axis) > 0.55F;
}
#endif

}  // namespace

void initializeGamepadMenuState(GamepadMenuState& state) {
#if defined(IGGY3D_HAS_SDL3)
  if (state.initialized) {
    return;
  }
  state.initialized = SDL_InitSubSystem(SDL_INIT_GAMEPAD);
  if (!state.initialized) {
    return;
  }
  int count = 0;
  SDL_JoystickID* ids = SDL_GetGamepads(&count);
  if (ids != nullptr && count > 0) {
    SDL_Gamepad* gamepad = SDL_OpenGamepad(ids[0]);
    state.nativeGamepad = gamepad;
    state.gamepadAvailable = gamepad != nullptr;
    if (gamepad != nullptr) {
      const char* name = SDL_GetGamepadName(gamepad);
      state.gamepadName = name == nullptr ? "sdl_gamepad" : name;
    }
  }
  SDL_free(ids);
#else
  (void)state;
#endif
}

void shutdownGamepadMenuState(GamepadMenuState& state) {
#if defined(IGGY3D_HAS_SDL3)
  if (state.nativeGamepad != nullptr) {
    SDL_CloseGamepad(nativeGamepad(state));
    state.nativeGamepad = nullptr;
  }
  if (state.initialized) {
    SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
    state.initialized = false;
  }
  state.gamepadAvailable = false;
#else
  (void)state;
#endif
}

InputAction pollGamepadMenuAction(GamepadMenuState& state) {
#if defined(IGGY3D_HAS_SDL3)
  if (!state.gamepadAvailable) {
    return InputAction::None;
  }
  const bool upDown = gamepadButtonDown(state, SDL_GAMEPAD_BUTTON_DPAD_UP);
  const bool downDown = gamepadButtonDown(state, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
  const bool leftDown = gamepadButtonDown(state, SDL_GAMEPAD_BUTTON_DPAD_LEFT);
  const bool rightDown = gamepadButtonDown(state, SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
  const bool confirmDown = gamepadButtonDown(state, SDL_GAMEPAD_BUTTON_SOUTH);
  const bool backDown = gamepadButtonDown(state, SDL_GAMEPAD_BUTTON_EAST);
  const bool optionsDown = gamepadButtonDown(state, SDL_GAMEPAD_BUTTON_START);

  InputAction action = InputAction::None;
  if (upDown && !state.upWasDown) {
    action = actionForInput(NeutralInput::DpadUp);
  } else if (downDown && !state.downWasDown) {
    action = actionForInput(NeutralInput::DpadDown);
  } else if (leftDown && !state.leftWasDown) {
    action = actionForInput(NeutralInput::DpadLeft);
  } else if (rightDown && !state.rightWasDown) {
    action = actionForInput(NeutralInput::DpadRight);
  } else if (confirmDown && !state.confirmWasDown) {
    action = actionForInput(NeutralInput::ButtonSouth);
  } else if (backDown && !state.backWasDown) {
    action = actionForInput(NeutralInput::ButtonEast);
  } else if (optionsDown && !state.optionsWasDown) {
    action = actionForInput(NeutralInput::Start);
  }

  state.upWasDown = upDown;
  state.downWasDown = downDown;
  state.leftWasDown = leftDown;
  state.rightWasDown = rightDown;
  state.confirmWasDown = confirmDown;
  state.backWasDown = backDown;
  state.optionsWasDown = optionsDown;
  return action;
#else
  (void)state;
  return InputAction::None;
#endif
}

void pollGamepadGameplayActions(GamepadMenuState& state, ActionState& actions) {
#if defined(IGGY3D_HAS_SDL3)
  if (!state.gamepadAvailable) {
    return;
  }

  const float moveX = normalizedAxis(state, SDL_GAMEPAD_AXIS_LEFTX);
  const float moveY = -normalizedAxis(state, SDL_GAMEPAD_AXIS_LEFTY);
  const float lookX = normalizedAxis(state, SDL_GAMEPAD_AXIS_RIGHTX);
  const float lookY = -normalizedAxis(state, SDL_GAMEPAD_AXIS_RIGHTY);
  const bool interactDown = gamepadButtonDown(state, SDL_GAMEPAD_BUTTON_SOUTH);
  const bool attackDown = triggerDown(state, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);

  if (moveX != 0.0F) {
    recordAction(actions, InputAction::PlayerMoveX, true, false, false, moveX);
  }
  if (moveY != 0.0F) {
    recordAction(actions, InputAction::PlayerMoveY, true, false, false, moveY);
  }
  if (lookX != 0.0F) {
    recordAction(actions, InputAction::PlayerLookX, true, false, false, lookX);
  }
  if (lookY != 0.0F) {
    recordAction(actions, InputAction::PlayerLookY, true, false, false, lookY);
  }
  if (interactDown && !state.gameplayInteractWasDown) {
    recordAction(actions, InputAction::PlayerInteract, true, true, false, 1.0F);
  }
  if (attackDown && !state.rightTriggerWasDown) {
    recordAction(actions, InputAction::PlayerAttack, true, true, false, 1.0F);
  }

  state.gameplayInteractWasDown = interactDown;
  state.rightTriggerWasDown = attackDown;
#else
  (void)state;
  (void)actions;
#endif
}

void recordGamepadRoomEditorActions(GamepadMenuState& state,
                                    const GamepadRoomEditorInputSample& sample,
                                    ActionState& actions) {
  if (sample.upDown && !state.editorUpWasDown) {
    recordAction(actions, InputAction::EditorNudgeZ, true, true, false, -1.0F);
  }
  if (sample.downDown && !state.editorDownWasDown) {
    recordAction(actions, InputAction::EditorNudgeZ, true, true, false, 1.0F);
  }
  if (sample.leftDown && !state.editorLeftWasDown) {
    recordAction(actions, InputAction::EditorNudgeX, true, true, false, -1.0F);
  }
  if (sample.rightDown && !state.editorRightWasDown) {
    recordAction(actions, InputAction::EditorNudgeX, true, true, false, 1.0F);
  }
  if (sample.placeDown && !state.editorPlaceWasDown) {
    recordAction(actions, InputAction::EditorPlace, true, true, false, 1.0F);
  }
  if (sample.nextToolDown && !state.editorNextToolWasDown) {
    recordAction(actions, InputAction::EditorNextTool, true, true, false, 1.0F);
  }
  if (sample.previousToolDown && !state.editorPreviousToolWasDown) {
    recordAction(actions, InputAction::EditorPreviousTool, true, true, false, 1.0F);
  }

  state.editorUpWasDown = sample.upDown;
  state.editorDownWasDown = sample.downDown;
  state.editorLeftWasDown = sample.leftDown;
  state.editorRightWasDown = sample.rightDown;
  state.editorPlaceWasDown = sample.placeDown;
  state.editorNextToolWasDown = sample.nextToolDown;
  state.editorPreviousToolWasDown = sample.previousToolDown;
}

void pollGamepadRoomEditorActions(GamepadMenuState& state, ActionState& actions) {
#if defined(IGGY3D_HAS_SDL3)
  if (!state.gamepadAvailable) {
    return;
  }

  GamepadRoomEditorInputSample sample;
  sample.upDown = gamepadButtonDown(state, SDL_GAMEPAD_BUTTON_DPAD_UP);
  sample.downDown = gamepadButtonDown(state, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
  sample.leftDown = gamepadButtonDown(state, SDL_GAMEPAD_BUTTON_DPAD_LEFT);
  sample.rightDown = gamepadButtonDown(state, SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
  sample.placeDown = gamepadButtonDown(state, SDL_GAMEPAD_BUTTON_SOUTH);
  sample.nextToolDown = gamepadButtonDown(state, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
  sample.previousToolDown = gamepadButtonDown(state, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
  recordGamepadRoomEditorActions(state, sample, actions);
#else
  (void)state;
  (void)actions;
#endif
}

}  // namespace iggy3d
