#include "app/input/KeyboardInput.hpp"

#include <array>

#include "app/input/ActionState.hpp"
#include "app/input/InputBindings.hpp"

#if defined(IGGY3D_HAS_SDL3)
#include <SDL3/SDL.h>
#endif

namespace iggy3d {
namespace {

constexpr std::array<char, 7> kAsciiRoomPaintGlyphs = {'#', '.', 'P', 'K',
                                                       '$', 'E', '+'};

struct KeyboardRoomEditorActionBinding {
  bool KeyboardRoomEditorInputSample::* down;
  bool KeyboardInputState::* wasDown;
  InputAction action;
  float value;
};

constexpr std::array kKeyboardRoomEditorActionBindings{
    KeyboardRoomEditorActionBinding{&KeyboardRoomEditorInputSample::upDown,
                                    &KeyboardInputState::editorUpWasDown,
                                    InputAction::EditorNudgeZ,
                                    -1.0F},
    KeyboardRoomEditorActionBinding{&KeyboardRoomEditorInputSample::downDown,
                                    &KeyboardInputState::editorDownWasDown,
                                    InputAction::EditorNudgeZ,
                                    1.0F},
    KeyboardRoomEditorActionBinding{&KeyboardRoomEditorInputSample::leftDown,
                                    &KeyboardInputState::editorLeftWasDown,
                                    InputAction::EditorNudgeX,
                                    -1.0F},
    KeyboardRoomEditorActionBinding{&KeyboardRoomEditorInputSample::rightDown,
                                    &KeyboardInputState::editorRightWasDown,
                                    InputAction::EditorNudgeX,
                                    1.0F},
    KeyboardRoomEditorActionBinding{&KeyboardRoomEditorInputSample::nextToolDown,
                                    &KeyboardInputState::editorNextToolWasDown,
                                    InputAction::EditorNextTool,
                                    1.0F},
    KeyboardRoomEditorActionBinding{
        &KeyboardRoomEditorInputSample::previousToolDown,
        &KeyboardInputState::editorPreviousToolWasDown,
        InputAction::EditorPreviousTool,
        1.0F},
    KeyboardRoomEditorActionBinding{
        &KeyboardRoomEditorInputSample::selectFloorToolDown,
        &KeyboardInputState::editorSelectFloorToolWasDown,
        InputAction::EditorSelectFloorTool,
        1.0F},
    KeyboardRoomEditorActionBinding{
        &KeyboardRoomEditorInputSample::selectWallToolDown,
        &KeyboardInputState::editorSelectWallToolWasDown,
        InputAction::EditorSelectWallTool,
        1.0F},
    KeyboardRoomEditorActionBinding{
        &KeyboardRoomEditorInputSample::rotateWallDirectionDown,
        &KeyboardInputState::editorRotateWallDirectionWasDown,
        InputAction::EditorRotateWallDirection,
        1.0F},
    KeyboardRoomEditorActionBinding{
        &KeyboardRoomEditorInputSample::previewPlacementDown,
        &KeyboardInputState::editorPreviewPlacementWasDown,
        InputAction::EditorPreviewPlacement,
        1.0F},
    KeyboardRoomEditorActionBinding{
        &KeyboardRoomEditorInputSample::confirmPreviewDown,
        &KeyboardInputState::editorConfirmPreviewWasDown,
        InputAction::EditorConfirmPreview,
        1.0F},
    KeyboardRoomEditorActionBinding{
        &KeyboardRoomEditorInputSample::cancelPreviewDown,
        &KeyboardInputState::editorCancelPreviewWasDown,
        InputAction::EditorCancelPreview,
        1.0F},
    KeyboardRoomEditorActionBinding{&KeyboardRoomEditorInputSample::placeDown,
                                    &KeyboardInputState::editorPlaceWasDown,
                                    InputAction::EditorPlace,
                                    1.0F},
    KeyboardRoomEditorActionBinding{&KeyboardRoomEditorInputSample::deleteDown,
                                    &KeyboardInputState::editorDeleteWasDown,
                                    InputAction::EditorDelete,
                                    1.0F},
    KeyboardRoomEditorActionBinding{&KeyboardRoomEditorInputSample::undoDown,
                                    &KeyboardInputState::editorUndoWasDown,
                                    InputAction::EditorUndo,
                                    1.0F},
    KeyboardRoomEditorActionBinding{&KeyboardRoomEditorInputSample::redoDown,
                                    &KeyboardInputState::editorRedoWasDown,
                                    InputAction::EditorRedo,
                                    1.0F},
};

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
  const bool devToggleDown = keyDown(keys, SDL_SCANCODE_F1) || keyDown(keys, SDL_SCANCODE_F2);

  InputAction action = InputAction::None;
  // branch-gate: BG-1037
  if (devToggleDown && !state.devToggleWasDown) {
    // branch-gate: BG-1037
    action = actionForInput(keyDown(keys, SDL_SCANCODE_F2) ? NeutralInput::KeyF2
                                                            : NeutralInput::KeyF1);
  } else if (upDown && !state.upWasDown) {  // branch-gate: BG-1037
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
  state.devToggleWasDown = devToggleDown;
  return action;
#else
  (void)state;
  return InputAction::None;
#endif
}

char recordKeyboardAsciiRoomPaintGlyph(KeyboardInputState& state,
                                       const KeyboardAsciiRoomPaintSample& sample) {
  char glyph = '\0';
  for (std::size_t index = 0; index < sample.glyphDown.size(); ++index) {
    const bool down = sample.glyphDown[index];
    // branch-gate: BG-1039
    if (down && !state.asciiPaintWasDown[index] && glyph == '\0') {
      glyph = kAsciiRoomPaintGlyphs[index];
    }
    state.asciiPaintWasDown[index] = down;
  }
  return glyph;
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
  KeyboardAsciiRoomPaintSample sample;
  for (std::size_t index = 0; index < kScanCodes.size(); ++index) {
    sample.glyphDown[index] = keyDown(keys, kScanCodes[index]);
  }
  return recordKeyboardAsciiRoomPaintGlyph(state, sample);
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
    recordAction(actions, InputAction::PlayerLookY, true, false, false, -1.0F);
  }
  if (lookDownDown) {
    recordAction(actions, InputAction::PlayerLookY, true, false, false, 1.0F);
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
  for (const KeyboardRoomEditorActionBinding& binding :
       kKeyboardRoomEditorActionBindings) {
    const bool down = sample.*(binding.down);
    bool& wasDown = state.*(binding.wasDown);
    // branch-gate: BG-1037
    if (down && !wasDown) {
      recordAction(actions, binding.action, true, true, false, binding.value);
    }
    wasDown = down;
  }
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
  sample.selectFloorToolDown = keyDown(keys, SDL_SCANCODE_1);
  sample.selectWallToolDown = keyDown(keys, SDL_SCANCODE_2);
  sample.rotateWallDirectionDown = keyDown(keys, SDL_SCANCODE_R);
  sample.previewPlacementDown = keyDown(keys, SDL_SCANCODE_F);
  sample.confirmPreviewDown = keyDown(keys, SDL_SCANCODE_RETURN);
  sample.cancelPreviewDown = keyDown(keys, SDL_SCANCODE_C);
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
