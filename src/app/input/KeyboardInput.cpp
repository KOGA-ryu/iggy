#include "app/input/KeyboardInput.hpp"

#include <array>

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
  const bool leftDown = keyDown(keys, SDL_SCANCODE_LEFT) || keyDown(keys, SDL_SCANCODE_A);
  const bool rightDown = keyDown(keys, SDL_SCANCODE_RIGHT) || keyDown(keys, SDL_SCANCODE_D);
  const bool confirmDown = keyDown(keys, SDL_SCANCODE_RETURN) || keyDown(keys, SDL_SCANCODE_SPACE);
  const bool backDown = keyDown(keys, SDL_SCANCODE_ESCAPE);
  const bool tabDown = keyDown(keys, SDL_SCANCODE_TAB);

  InputAction action = InputAction::None;
  if (upDown && !state.upWasDown) {
    action = actionForInput(NeutralInput::KeyUp);
  } else if (downDown && !state.downWasDown) {
    action = actionForInput(NeutralInput::KeyDown);
  } else if (leftDown && !state.leftWasDown) {
    action = actionForInput(NeutralInput::KeyLeft);
  } else if (rightDown && !state.rightWasDown) {
    action = actionForInput(NeutralInput::KeyRight);
  } else if (confirmDown && !state.confirmWasDown) {
    action = actionForInput(keyDown(keys, SDL_SCANCODE_RETURN) ? NeutralInput::KeyEnter
                                                                : NeutralInput::KeySpace);
  } else if (backDown && !state.backWasDown) {
    action = actionForInput(NeutralInput::KeyEscape);
  } else if (tabDown && !state.tabWasDown) {
    action = actionForInput(NeutralInput::KeyTab);
  }

  state.upWasDown = upDown;
  state.downWasDown = downDown;
  state.leftWasDown = leftDown;
  state.rightWasDown = rightDown;
  state.confirmWasDown = confirmDown;
  state.backWasDown = backDown;
  state.tabWasDown = tabDown;
  return action;
#else
  (void)state;
  return InputAction::None;
#endif
}

char pollKeyboardAsciiRoomPaintGlyph(KeyboardInputState& state) {
#if defined(IGGY3D_HAS_SDL3)
  const bool* keys = SDL_GetKeyboardState(nullptr);
  constexpr std::array<SDL_Scancode, 7> kScanCodes = {
      SDL_SCANCODE_1,
      SDL_SCANCODE_2,
      SDL_SCANCODE_3,
      SDL_SCANCODE_4,
      SDL_SCANCODE_5,
      SDL_SCANCODE_6,
      SDL_SCANCODE_7,
  };
  constexpr std::array<char, 7> kGlyphs = {'#', '.', 'P', 'K', '$', 'E', '+'};
  char glyph = '\0';
  for (std::size_t index = 0; index < kScanCodes.size(); ++index) {
    const bool down = keyDown(keys, kScanCodes[index]);
    if (down && !state.asciiPaintWasDown[index] && glyph == '\0') {
      glyph = kGlyphs[index];
    }
    state.asciiPaintWasDown[index] = down;
  }
  return glyph;
#else
  (void)state;
  return '\0';
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

void recordKeyboardRoomEditorActions(KeyboardInputState& state,
                                     const KeyboardRoomEditorInputSample& sample,
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
  if (sample.nextToolDown && !state.editorNextToolWasDown) {
    recordAction(actions, InputAction::EditorNextTool, true, true, false, 1.0F);
  }
  if (sample.previousToolDown && !state.editorPreviousToolWasDown) {
    recordAction(actions, InputAction::EditorPreviousTool, true, true, false, 1.0F);
  }
  if (sample.placeDown && !state.editorPlaceWasDown) {
    recordAction(actions, InputAction::EditorPlace, true, true, false, 1.0F);
  }
  // branch-gate: BG-1037
  if (sample.deleteDown && !state.editorDeleteWasDown) {
    recordAction(actions, InputAction::EditorDelete, true, true, false, 1.0F);
  }
  // branch-gate: BG-1037
  if (sample.undoDown && !state.editorUndoWasDown) {
    recordAction(actions, InputAction::EditorUndo, true, true, false, 1.0F);
  }
  // branch-gate: BG-1037
  if (sample.redoDown && !state.editorRedoWasDown) {
    recordAction(actions, InputAction::EditorRedo, true, true, false, 1.0F);
  }

  state.editorUpWasDown = sample.upDown;
  state.editorDownWasDown = sample.downDown;
  state.editorLeftWasDown = sample.leftDown;
  state.editorRightWasDown = sample.rightDown;
  state.editorNextToolWasDown = sample.nextToolDown;
  state.editorPreviousToolWasDown = sample.previousToolDown;
  state.editorPlaceWasDown = sample.placeDown;
  state.editorDeleteWasDown = sample.deleteDown;
  state.editorUndoWasDown = sample.undoDown;
  state.editorRedoWasDown = sample.redoDown;
}

void pollKeyboardRoomEditorActions(KeyboardInputState& state, ActionState& actions) {
#if defined(IGGY3D_HAS_SDL3)
  const bool* keys = SDL_GetKeyboardState(nullptr);
  KeyboardRoomEditorInputSample sample;
  sample.upDown = keyDown(keys, SDL_SCANCODE_W);
  sample.downDown = keyDown(keys, SDL_SCANCODE_S);
  sample.leftDown = keyDown(keys, SDL_SCANCODE_A);
  sample.rightDown = keyDown(keys, SDL_SCANCODE_D);
  sample.nextToolDown = keyDown(keys, SDL_SCANCODE_E);
  sample.previousToolDown = keyDown(keys, SDL_SCANCODE_Q);
  sample.placeDown = keyDown(keys, SDL_SCANCODE_SPACE);
  sample.deleteDown = keyDown(keys, SDL_SCANCODE_DELETE);
  sample.undoDown = keyDown(keys, SDL_SCANCODE_Z);
  sample.redoDown = keyDown(keys, SDL_SCANCODE_Y);
  recordKeyboardRoomEditorActions(state, sample, actions);
#else
  (void)state;
  (void)actions;
#endif
}

}  // namespace iggy3d
