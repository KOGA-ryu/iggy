#include "app/input/MouseInput.hpp"

#include "app/input/ActionState.hpp"
#include "app/input/InputActions.hpp"

#if defined(IGGY3D_HAS_SDL3)
#include <SDL3/SDL.h>
#endif

namespace iggy3d {

MouseClick pollMouseClick(MouseInputState& state) {
#if defined(IGGY3D_HAS_SDL3)
  float x = 0.0F;
  float y = 0.0F;
  const SDL_MouseButtonFlags buttons = SDL_GetMouseState(&x, &y);
  const bool leftDown = (buttons & SDL_BUTTON_LMASK) != 0U;
  MouseClick click;
  click.clicked = leftDown && !state.leftWasDown;
  click.x = x;
  click.y = y;
  state.leftWasDown = leftDown;
  return click;
#else
  (void)state;
  return {};
#endif
}

void pollMouseGameplayActions(MouseInputState& state, ActionState& actions) {
#if defined(IGGY3D_HAS_SDL3)
  (void)state;
  float deltaX = 0.0F;
  float deltaY = 0.0F;
  SDL_GetRelativeMouseState(&deltaX, &deltaY);
  constexpr float kMouseDeltaScale = 0.02F;
  if (deltaX != 0.0F) {
    recordAction(actions, actionForInput(NeutralInput::MouseDeltaX), true, false, false,
                 deltaX * kMouseDeltaScale);
  }
  if (deltaY != 0.0F) {
    recordAction(actions, actionForInput(NeutralInput::MouseDeltaY), true, false, false,
                 deltaY * kMouseDeltaScale);
  }
#else
  (void)state;
  (void)actions;
#endif
}

InputAction mouseClickAction(const MouseClick& click) {
  return click.clicked ? actionForInput(NeutralInput::MouseLeft) : InputAction::None;
}

}  // namespace iggy3d
