#include "app/input/ActionState.hpp"

namespace iggy3d {

void clearActionState(ActionState& state) {
  state.entries.clear();
}

void recordAction(ActionState& state,
                  InputAction action,
                  bool down,
                  bool pressed,
                  bool released,
                  float value) {
  if (action == InputAction::None) {
    return;
  }
  for (ActionStateEntry& entry : state.entries) {
    if (entry.action == action) {
      entry.down = entry.down || down;
      entry.pressed = entry.pressed || pressed;
      entry.released = entry.released || released;
      entry.value += value;
      return;
    }
  }
  state.entries.push_back({action, down, pressed, released, value});
}

bool actionIsDown(const ActionState& state, InputAction action) {
  for (const ActionStateEntry& entry : state.entries) {
    if (entry.action == action) {
      return entry.down;
    }
  }
  return false;
}

bool actionWasPressed(const ActionState& state, InputAction action) {
  for (const ActionStateEntry& entry : state.entries) {
    if (entry.action == action) {
      return entry.pressed;
    }
  }
  return false;
}

bool actionWasReleased(const ActionState& state, InputAction action) {
  for (const ActionStateEntry& entry : state.entries) {
    if (entry.action == action) {
      return entry.released;
    }
  }
  return false;
}

float actionAxisValue(const ActionState& state, InputAction action) {
  for (const ActionStateEntry& entry : state.entries) {
    if (entry.action == action) {
      return entry.value;
    }
  }
  return 0.0F;
}

}  // namespace iggy3d
