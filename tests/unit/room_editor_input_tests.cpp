#include "app/input/GamepadInput.hpp"
#include "app/input/KeyboardInput.hpp"

#include <cmath>
#include <iostream>
#include <string_view>

#include "app/input/ActionState.hpp"

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool expectActionValue(const iggy3d::ActionState& actions,
                       iggy3d::InputAction action,
                       float expected,
                       std::string_view message) {
  return expect(actions.entries.size() == 1, message) &&
         expect(actions.entries.front().action == action, message) &&
         expect(actions.entries.front().down, message) &&
         expect(actions.entries.front().pressed, message) &&
         expect(std::fabs(actions.entries.front().value - expected) < 0.001F,
                message);
}

}  // namespace

int main() {
  bool ok = true;

  {
    iggy3d::KeyboardInputState keyboard;
    iggy3d::ActionState actions;
    iggy3d::KeyboardRoomEditorInputSample sample;
    sample.upDown = true;
    iggy3d::recordKeyboardRoomEditorActions(keyboard, sample, actions);
    ok = expectActionValue(actions, iggy3d::InputAction::EditorNudgeZ, -1.0F,
                           "keyboard W emits editor nudge z negative") &&
         ok;
  }

  {
    iggy3d::KeyboardInputState keyboard;
    iggy3d::ActionState actions;
    iggy3d::KeyboardRoomEditorInputSample sample;
    sample.downDown = true;
    iggy3d::recordKeyboardRoomEditorActions(keyboard, sample, actions);
    ok = expectActionValue(actions, iggy3d::InputAction::EditorNudgeZ, 1.0F,
                           "keyboard S emits editor nudge z positive") &&
         ok;
  }

  {
    iggy3d::KeyboardInputState keyboard;
    iggy3d::ActionState actions;
    iggy3d::KeyboardRoomEditorInputSample sample;
    sample.leftDown = true;
    iggy3d::recordKeyboardRoomEditorActions(keyboard, sample, actions);
    ok = expectActionValue(actions, iggy3d::InputAction::EditorNudgeX, -1.0F,
                           "keyboard A emits editor nudge x negative") &&
         ok;
  }

  {
    iggy3d::KeyboardInputState keyboard;
    iggy3d::ActionState actions;
    iggy3d::KeyboardRoomEditorInputSample sample;
    sample.rightDown = true;
    iggy3d::recordKeyboardRoomEditorActions(keyboard, sample, actions);
    ok = expectActionValue(actions, iggy3d::InputAction::EditorNudgeX, 1.0F,
                           "keyboard D emits editor nudge x positive") &&
         ok;
  }

  {
    iggy3d::KeyboardInputState keyboard;
    iggy3d::KeyboardRoomEditorInputSample sample;
    sample.rightDown = true;

    iggy3d::ActionState first;
    iggy3d::recordKeyboardRoomEditorActions(keyboard, sample, first);
    iggy3d::ActionState held;
    iggy3d::recordKeyboardRoomEditorActions(keyboard, sample, held);
    sample.rightDown = false;
    iggy3d::ActionState released;
    iggy3d::recordKeyboardRoomEditorActions(keyboard, sample, released);
    sample.rightDown = true;
    iggy3d::ActionState pressedAgain;
    iggy3d::recordKeyboardRoomEditorActions(keyboard, sample, pressedAgain);

    ok = expect(first.entries.size() == 1, "keyboard first press emits") && ok;
    ok = expect(held.entries.empty(), "keyboard held key does not repeat") && ok;
    ok = expect(released.entries.empty(), "keyboard release emits no nudge") && ok;
    ok = expectActionValue(pressedAgain, iggy3d::InputAction::EditorNudgeX, 1.0F,
                           "keyboard re-press emits after release") &&
         ok;
  }

  {
    iggy3d::KeyboardInputState keyboard;
    iggy3d::ActionState nextActions;
    iggy3d::KeyboardRoomEditorInputSample next;
    next.nextToolDown = true;
    iggy3d::recordKeyboardRoomEditorActions(keyboard, next, nextActions);
    ok = expectActionValue(nextActions, iggy3d::InputAction::EditorNextTool, 1.0F,
                           "keyboard E emits next tool") &&
         ok;

    next.nextToolDown = false;
    iggy3d::ActionState release;
    iggy3d::recordKeyboardRoomEditorActions(keyboard, next, release);
    iggy3d::ActionState previousActions;
    next.previousToolDown = true;
    iggy3d::recordKeyboardRoomEditorActions(keyboard, next, previousActions);
    ok = expectActionValue(previousActions,
                           iggy3d::InputAction::EditorPreviousTool,
                           1.0F,
                           "keyboard Q emits previous tool") &&
         ok;
  }

  {
    iggy3d::KeyboardInputState keyboard;
    iggy3d::ActionState actions;
    iggy3d::KeyboardRoomEditorInputSample sample;
    sample.placeDown = true;
    iggy3d::recordKeyboardRoomEditorActions(keyboard, sample, actions);
    ok = expectActionValue(actions, iggy3d::InputAction::EditorPlace, 1.0F,
                           "keyboard space emits place") &&
         ok;
  }

  {
    iggy3d::KeyboardInputState keyboard;
    iggy3d::ActionState actions;
    iggy3d::KeyboardRoomEditorInputSample sample;
    sample.deleteDown = true;
    iggy3d::recordKeyboardRoomEditorActions(keyboard, sample, actions);
    ok = expectActionValue(actions, iggy3d::InputAction::EditorDelete, 1.0F,
                           "keyboard delete emits editor delete") &&
         ok;
  }

  {
    iggy3d::KeyboardInputState keyboard;
    iggy3d::ActionState actions;
    iggy3d::KeyboardRoomEditorInputSample sample;
    sample.undoDown = true;
    iggy3d::recordKeyboardRoomEditorActions(keyboard, sample, actions);
    ok = expectActionValue(actions, iggy3d::InputAction::EditorUndo, 1.0F,
                           "keyboard Z emits editor undo") &&
         ok;
  }

  {
    iggy3d::KeyboardInputState keyboard;
    iggy3d::ActionState actions;
    iggy3d::KeyboardRoomEditorInputSample sample;
    sample.redoDown = true;
    iggy3d::recordKeyboardRoomEditorActions(keyboard, sample, actions);
    ok = expectActionValue(actions, iggy3d::InputAction::EditorRedo, 1.0F,
                           "keyboard Y emits editor redo") &&
         ok;
  }

  {
    iggy3d::GamepadMenuState gamepad;
    iggy3d::ActionState actions;
    iggy3d::GamepadRoomEditorInputSample sample;
    sample.upDown = true;
    iggy3d::recordGamepadRoomEditorActions(gamepad, sample, actions);
    ok = expectActionValue(actions, iggy3d::InputAction::EditorNudgeZ, -1.0F,
                           "gamepad dpad up emits editor nudge z negative") &&
         ok;
  }

  {
    iggy3d::GamepadMenuState gamepad;
    iggy3d::ActionState actions;
    iggy3d::GamepadRoomEditorInputSample sample;
    sample.rightDown = true;
    iggy3d::recordGamepadRoomEditorActions(gamepad, sample, actions);
    ok = expectActionValue(actions, iggy3d::InputAction::EditorNudgeX, 1.0F,
                           "gamepad dpad right emits editor nudge x positive") &&
         ok;
  }

  {
    iggy3d::GamepadMenuState gamepad;
    iggy3d::ActionState placeActions;
    iggy3d::GamepadRoomEditorInputSample sample;
    sample.placeDown = true;
    iggy3d::recordGamepadRoomEditorActions(gamepad, sample, placeActions);
    ok = expectActionValue(placeActions, iggy3d::InputAction::EditorPlace, 1.0F,
                           "gamepad south emits place") &&
         ok;

    sample.placeDown = false;
    iggy3d::ActionState release;
    iggy3d::recordGamepadRoomEditorActions(gamepad, sample, release);
    sample.nextToolDown = true;
    iggy3d::ActionState nextActions;
    iggy3d::recordGamepadRoomEditorActions(gamepad, sample, nextActions);
    ok = expectActionValue(nextActions, iggy3d::InputAction::EditorNextTool, 1.0F,
                           "gamepad right shoulder emits next tool") &&
         ok;

    sample.nextToolDown = false;
    iggy3d::ActionState nextRelease;
    iggy3d::recordGamepadRoomEditorActions(gamepad, sample, nextRelease);
    sample.previousToolDown = true;
    iggy3d::ActionState previousActions;
    iggy3d::recordGamepadRoomEditorActions(gamepad, sample, previousActions);
    ok = expectActionValue(previousActions,
                           iggy3d::InputAction::EditorPreviousTool,
                           1.0F,
                           "gamepad left shoulder emits previous tool") &&
         ok;
  }

  {
    iggy3d::GamepadMenuState gamepad;
    iggy3d::GamepadRoomEditorInputSample sample;
    sample.downDown = true;
    iggy3d::ActionState first;
    iggy3d::recordGamepadRoomEditorActions(gamepad, sample, first);
    iggy3d::ActionState held;
    iggy3d::recordGamepadRoomEditorActions(gamepad, sample, held);
    ok = expect(first.entries.size() == 1, "gamepad first press emits") && ok;
    ok = expect(held.entries.empty(), "gamepad held button does not repeat") && ok;
  }

  return ok ? 0 : 1;
}
