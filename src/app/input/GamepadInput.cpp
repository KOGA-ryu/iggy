#include "app/input/GamepadInput.hpp"

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
  const bool confirmDown = gamepadButtonDown(state, SDL_GAMEPAD_BUTTON_SOUTH);
  const bool backDown = gamepadButtonDown(state, SDL_GAMEPAD_BUTTON_EAST);
  const bool optionsDown = gamepadButtonDown(state, SDL_GAMEPAD_BUTTON_START);

  InputAction action = InputAction::None;
  if (upDown && !state.upWasDown) {
    action = actionForInput(NeutralInput::DpadUp);
  } else if (downDown && !state.downWasDown) {
    action = actionForInput(NeutralInput::DpadDown);
  } else if (confirmDown && !state.confirmWasDown) {
    action = actionForInput(NeutralInput::ButtonSouth);
  } else if (backDown && !state.backWasDown) {
    action = actionForInput(NeutralInput::ButtonEast);
  } else if (optionsDown && !state.optionsWasDown) {
    action = actionForInput(NeutralInput::Start);
  }

  state.upWasDown = upDown;
  state.downWasDown = downDown;
  state.confirmWasDown = confirmDown;
  state.backWasDown = backDown;
  state.optionsWasDown = optionsDown;
  return action;
#else
  (void)state;
  return InputAction::None;
#endif
}

}  // namespace iggy3d
