#include "app/input/GamepadInput.hpp"
#include "app/input/InputBindings.hpp"
#include "app/input/KeyboardInput.hpp"

#include <array>
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
    ok = expect(iggy3d::actionForInput(iggy3d::NeutralInput::KeyTab) ==
                    iggy3d::InputAction::MenuNextTab,
                "keyboard Tab emits menu next tab") &&
         ok;
    ok = expect(iggy3d::actionForInput(iggy3d::NeutralInput::KeyW) ==
                    iggy3d::InputAction::PlayerMoveY,
                "keyboard W remains gameplay move outside menu polling") &&
         ok;
    ok = expect(iggy3d::actionForInput(iggy3d::NeutralInput::KeyDown) ==
                    iggy3d::InputAction::MenuDown,
                "keyboard Down emits menu down") &&
         ok;
    ok = expect(iggy3d::actionForInput(iggy3d::NeutralInput::KeyRight) ==
                    iggy3d::InputAction::MenuRight,
                "keyboard Right emits menu right") &&
         ok;
    ok = expect(iggy3d::actionForInput(iggy3d::NeutralInput::KeyEnter) ==
                    iggy3d::InputAction::MenuConfirm,
                "keyboard Enter emits menu confirm") &&
         ok;
    ok = expect(iggy3d::actionForInput(iggy3d::NeutralInput::KeySpace) ==
                    iggy3d::InputAction::MenuConfirm,
                "keyboard Space emits menu confirm") &&
         ok;
    ok = expect(iggy3d::actionForInput(iggy3d::NeutralInput::KeyF1) ==
                    iggy3d::InputAction::DevToggle,
                "keyboard F1 emits dev toggle") &&
         ok;
    ok = expect(iggy3d::neutralInputName(iggy3d::NeutralInput::KeyF2) ==
                    "key.f2",
                "keyboard F2 name") &&
         ok;
    ok = expect(iggy3d::actionForInput(iggy3d::NeutralInput::KeyF2) ==
                    iggy3d::InputAction::DevCollisionOverlay,
                "keyboard F2 emits collision overlay toggle") &&
         ok;
    ok = expect(iggy3d::neutralInputName(iggy3d::NeutralInput::KeyF3) ==
                    "key.f3",
                "keyboard F3 name") &&
         ok;
    ok = expect(iggy3d::actionForInput(iggy3d::NeutralInput::KeyF3) ==
                    iggy3d::InputAction::DevDebugOverlay,
                "keyboard F3 emits debug overlay toggle") &&
         ok;
    ok = expect(iggy3d::neutralInputName(iggy3d::NeutralInput::KeyF4) ==
                    "key.f4",
                "keyboard F4 name") &&
         ok;
    ok = expect(iggy3d::actionForInput(iggy3d::NeutralInput::KeyF4) ==
                    iggy3d::InputAction::MovementTuningToggle,
                "keyboard F4 emits movement tuning toggle") &&
         ok;
  }

  {
    constexpr std::array<char, iggy3d::kKeyboardAsciiRoomPaintGlyphCount> expectedGlyphs = {
        '#', '.', 'P', 'K', '$', 'E', '+', 'C', '^', 'v', '<', '>', 'R'};
    for (std::size_t index = 0; index < expectedGlyphs.size(); ++index) {
      iggy3d::KeyboardInputState keyboard;
      iggy3d::KeyboardAsciiRoomPaintSample sample;
      sample.glyphDown[index] = true;
      const char first = iggy3d::recordKeyboardAsciiRoomPaintGlyph(keyboard, sample);
      const char held = iggy3d::recordKeyboardAsciiRoomPaintGlyph(keyboard, sample);
      sample.glyphDown[index] = false;
      const char released =
          iggy3d::recordKeyboardAsciiRoomPaintGlyph(keyboard, sample);
      sample.glyphDown[index] = true;
      const char pressedAgain =
          iggy3d::recordKeyboardAsciiRoomPaintGlyph(keyboard, sample);

      ok = expect(first == expectedGlyphs[index],
                  "keyboard number key emits expected draft glyph") &&
           ok;
      ok = expect(held == '\0', "keyboard held draft paint key does not repeat") &&
           ok;
      ok = expect(released == '\0',
                  "keyboard draft paint key release emits no glyph") &&
           ok;
      ok = expect(pressedAgain == expectedGlyphs[index],
                  "keyboard draft paint key re-press emits expected glyph") &&
           ok;
    }
  }

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
    iggy3d::ActionState floorActions;
    iggy3d::KeyboardRoomEditorInputSample sample;
    sample.selectFloorToolDown = true;
    iggy3d::recordKeyboardRoomEditorActions(keyboard, sample, floorActions);
    ok = expectActionValue(floorActions,
                           iggy3d::InputAction::EditorSelectFloorTool,
                           1.0F,
                           "keyboard 1 emits direct floor tool") &&
         ok;

    iggy3d::ActionState held;
    iggy3d::recordKeyboardRoomEditorActions(keyboard, sample, held);
    ok = expect(held.entries.empty(), "keyboard held direct floor does not repeat") &&
         ok;

    sample.selectFloorToolDown = false;
    iggy3d::ActionState release;
    iggy3d::recordKeyboardRoomEditorActions(keyboard, sample, release);
    sample.selectWallToolDown = true;
    iggy3d::ActionState wallActions;
    iggy3d::recordKeyboardRoomEditorActions(keyboard, sample, wallActions);
    ok = expectActionValue(wallActions,
                           iggy3d::InputAction::EditorSelectWallTool,
                           1.0F,
                           "keyboard 2 emits direct wall tool") &&
         ok;
  }

  {
    iggy3d::KeyboardInputState keyboard;
    iggy3d::ActionState actions;
    iggy3d::KeyboardRoomEditorInputSample sample;
    sample.rotateWallDirectionDown = true;
    iggy3d::recordKeyboardRoomEditorActions(keyboard, sample, actions);
    ok = expectActionValue(actions,
                           iggy3d::InputAction::EditorRotateWallDirection,
                           1.0F,
                           "keyboard R emits wall direction rotate") &&
         ok;

    iggy3d::ActionState held;
    iggy3d::recordKeyboardRoomEditorActions(keyboard, sample, held);
    ok = expect(held.entries.empty(),
                "keyboard held wall direction rotate does not repeat") &&
         ok;
  }

  {
    iggy3d::KeyboardInputState keyboard;
    iggy3d::ActionState previewActions;
    iggy3d::KeyboardRoomEditorInputSample sample;
    sample.previewPlacementDown = true;
    iggy3d::recordKeyboardRoomEditorActions(keyboard, sample, previewActions);
    ok = expectActionValue(previewActions,
                           iggy3d::InputAction::EditorPreviewPlacement,
                           1.0F,
                           "keyboard F emits editor preview") &&
         ok;

    iggy3d::ActionState held;
    iggy3d::recordKeyboardRoomEditorActions(keyboard, sample, held);
    ok = expect(held.entries.empty(),
                "keyboard held editor preview does not repeat") &&
         ok;

    sample.previewPlacementDown = false;
    iggy3d::ActionState release;
    iggy3d::recordKeyboardRoomEditorActions(keyboard, sample, release);
    sample.confirmPreviewDown = true;
    iggy3d::ActionState confirmActions;
    iggy3d::recordKeyboardRoomEditorActions(keyboard, sample, confirmActions);
    ok = expectActionValue(confirmActions,
                           iggy3d::InputAction::EditorConfirmPreview,
                           1.0F,
                           "keyboard Enter emits editor preview confirm") &&
         ok;

    sample.confirmPreviewDown = false;
    iggy3d::ActionState confirmRelease;
    iggy3d::recordKeyboardRoomEditorActions(keyboard, sample, confirmRelease);
    sample.cancelPreviewDown = true;
    iggy3d::ActionState cancelActions;
    iggy3d::recordKeyboardRoomEditorActions(keyboard, sample, cancelActions);
    ok = expectActionValue(cancelActions,
                           iggy3d::InputAction::EditorCancelPreview,
                           1.0F,
                           "keyboard C emits editor preview cancel") &&
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
