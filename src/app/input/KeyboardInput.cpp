#include "app/input/KeyboardInput.hpp"

#include "app/input/InputBindings.hpp"

#if defined(IGGY3D_HAS_SDL3)
#include <SDL3/SDL.h>
#endif

namespace iggy3d {
namespace {

#if defined(IGGY3D_HAS_SDL3)
bool keyDown(const bool* keys, SDL_Scancode scanCode) {
  return keys != nullptr && keys[scanCode];
}
#endif

}  // namespace

InputAction pollKeyboardMenuAction(KeyboardInputState& state) {
#if defined(IGGY3D_HAS_SDL3)
  const bool* keys = SDL_GetKeyboardState(nullptr);
  const bool upDown = keyDown(keys, SDL_SCANCODE_UP) || keyDown(keys, SDL_SCANCODE_W);
  const bool downDown = keyDown(keys, SDL_SCANCODE_DOWN) || keyDown(keys, SDL_SCANCODE_S);
  const bool confirmDown = keyDown(keys, SDL_SCANCODE_RETURN) || keyDown(keys, SDL_SCANCODE_SPACE);
  const bool backDown = keyDown(keys, SDL_SCANCODE_ESCAPE);

  InputAction action = InputAction::None;
  if (upDown && !state.upWasDown) {
    action = actionForInput(NeutralInput::KeyUp);
  } else if (downDown && !state.downWasDown) {
    action = actionForInput(NeutralInput::KeyDown);
  } else if (confirmDown && !state.confirmWasDown) {
    action = actionForInput(keyDown(keys, SDL_SCANCODE_RETURN) ? NeutralInput::KeyEnter
                                                                : NeutralInput::KeySpace);
  } else if (backDown && !state.backWasDown) {
    action = actionForInput(NeutralInput::KeyEscape);
  }

  state.upWasDown = upDown;
  state.downWasDown = downDown;
  state.confirmWasDown = confirmDown;
  state.backWasDown = backDown;
  return action;
#else
  (void)state;
  return InputAction::None;
#endif
}

}  // namespace iggy3d
