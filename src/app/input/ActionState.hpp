#pragma once

#include <vector>

#include "app/input/InputActions.hpp"

namespace iggy3d {

struct ActionStateEntry {
  InputAction action = InputAction::None;
  bool down = false;
  bool pressed = false;
  bool released = false;
  float value = 0.0F;
};

struct ActionState {
  std::vector<ActionStateEntry> entries;
};

void clearActionState(ActionState& state);
void recordAction(ActionState& state,
                  InputAction action,
                  bool down,
                  bool pressed,
                  bool released,
                  float value);
bool actionIsDown(const ActionState& state, InputAction action);
bool actionWasPressed(const ActionState& state, InputAction action);
bool actionWasReleased(const ActionState& state, InputAction action);
float actionAxisValue(const ActionState& state, InputAction action);

}  // namespace iggy3d
