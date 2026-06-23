#include "app/input/KeyboardInput.hpp"

#include "app/input/ActionState.hpp"
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

void pollKeyboardGameplayActions(KeyboardInputState& state, ActionState& actions) {
#if defined(IGGY3D_HAS_SDL3)
  const bool* keys = SDL_GetKeyboardState(nullptr);
  const bool forwardDown = keyDown(keys, SDL_SCANCODE_W);
  const bool backDown = keyDown(keys, SDL_SCANCODE_S);
  const bool leftDown = keyDown(keys, SDL_SCANCODE_A);
  const bool rightDown = keyDown(keys, SDL_SCANCODE_D);
  const bool lookLeftDown = keyDown(keys, SDL_SCANCODE_LEFT);
  const bool lookRightDown = keyDown(keys, SDL_SCANCODE_RIGHT);
  const bool lookUpDown = keyDown(keys, SDL_SCANCODE_UP);
  const bool lookDownDown = keyDown(keys, SDL_SCANCODE_DOWN);
  const bool interactDown = keyDown(keys, SDL_SCANCODE_E);
  const bool retryDown = keyDown(keys, SDL_SCANCODE_R);

  if (forwardDown) {
    recordAction(actions, InputAction::PlayerMoveY, true, false, false, 1.0F);
  }
  if (backDown) {
    recordAction(actions, InputAction::PlayerMoveY, true, false, false, -1.0F);
  }
  if (leftDown) {
    recordAction(actions, InputAction::PlayerMoveX, true, false, false, -1.0F);
  }
  if (rightDown) {
    recordAction(actions, InputAction::PlayerMoveX, true, false, false, 1.0F);
  }
  if (lookLeftDown) {
    recordAction(actions, InputAction::PlayerLookX, true, false, false, -1.0F);
  }
  if (lookRightDown) {
    recordAction(actions, InputAction::PlayerLookX, true, false, false, 1.0F);
  }
  if (lookUpDown) {
    recordAction(actions, InputAction::PlayerLookY, true, false, false, 1.0F);
  }
  if (lookDownDown) {
    recordAction(actions, InputAction::PlayerLookY, true, false, false, -1.0F);
  }
  if (interactDown && !state.interactWasDown) {
    recordAction(actions, InputAction::PlayerInteract, true, true, false, 1.0F);
  }
  if (retryDown && !state.retryWasDown) {
    recordAction(actions, InputAction::PlayerRetryOrReset, true, true, false, 1.0F);
  }

  state.interactWasDown = interactDown;
  state.retryWasDown = retryDown;
#else
  (void)state;
  (void)actions;
#endif
}

}  // namespace iggy3d
