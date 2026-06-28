#include "app/input/KeyboardInput.hpp"

#include <array>

#include "app/input/ActionState.hpp"
#include "app/input/InputBindings.hpp"

#if defined(IGGY3D_HAS_SDL3)
#include <SDL3/SDL.h>
#endif

namespace iggy3d {
namespace {

constexpr std::array<char, kKeyboardAsciiRoomPaintGlyphCount> kAsciiRoomPaintGlyphs = {
    '#', '.', 'P', 'K', '$', 'E', '+', 'C', '^', 'v', '<', '>', 'R'};

struct KeyboardRoomEditorActionBinding {
  bool KeyboardRoomEditorInputSample::* down;
  bool KeyboardInputState::* wasDown;
  InputAction action;
  float value;
};

struct KeyboardMenuActionBinding {
  bool KeyboardMenuInputSample::* down = nullptr;
  bool KeyboardInputState::* wasDown = nullptr;
  NeutralInput input = NeutralInput::None;
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

static constexpr std::array kKeyboardMenuActionBindings{
    KeyboardMenuActionBinding{&KeyboardMenuInputSample::debugOverlayDown,
                              &KeyboardInputState::debugOverlayWasDown,
                              NeutralInput::KeyF3},
    KeyboardMenuActionBinding{&KeyboardMenuInputSample::movementTuningToggleDown,
                              &KeyboardInputState::movementTuningToggleWasDown,
                              NeutralInput::KeyF4},
    KeyboardMenuActionBinding{&KeyboardMenuInputSample::devToggleDown,
                              &KeyboardInputState::devToggleWasDown,
                              NeutralInput::KeyF1},
    KeyboardMenuActionBinding{&KeyboardMenuInputSample::devCollisionOverlayDown,
                              &KeyboardInputState::devCollisionOverlayWasDown,
                              NeutralInput::KeyF2},
    KeyboardMenuActionBinding{&KeyboardMenuInputSample::mapMakerToggleDown,
                              &KeyboardInputState::mapMakerToggleWasDown,
                              NeutralInput::KeyM},
    KeyboardMenuActionBinding{&KeyboardMenuInputSample::upDown,
                              &KeyboardInputState::upWasDown,
                              NeutralInput::KeyUp},
    KeyboardMenuActionBinding{&KeyboardMenuInputSample::downDown,
                              &KeyboardInputState::downWasDown,
                              NeutralInput::KeyDown},
    KeyboardMenuActionBinding{&KeyboardMenuInputSample::leftDown,
                              &KeyboardInputState::leftWasDown,
                              NeutralInput::KeyLeft},
    KeyboardMenuActionBinding{&KeyboardMenuInputSample::rightDown,
                              &KeyboardInputState::rightWasDown,
                              NeutralInput::KeyRight},
    KeyboardMenuActionBinding{&KeyboardMenuInputSample::confirmDown,
                              &KeyboardInputState::confirmWasDown,
                              NeutralInput::KeyEnter},
    KeyboardMenuActionBinding{&KeyboardMenuInputSample::backDown,
                              &KeyboardInputState::backWasDown,
                              NeutralInput::KeyEscape},
    KeyboardMenuActionBinding{&KeyboardMenuInputSample::tabDown,
                              &KeyboardInputState::tabWasDown,
                              NeutralInput::KeyTab},
};

#if defined(IGGY3D_HAS_SDL3)
bool keyDown(const bool* keys, SDL_Scancode scanCode) {
  return keys != nullptr && keys[scanCode];
}
#endif

}  // namespace

InputAction recordKeyboardMenuAction(KeyboardInputState& state,
                                     const KeyboardMenuInputSample& sample) {
  InputAction action = InputAction::None;
  for (const KeyboardMenuActionBinding& binding : kKeyboardMenuActionBindings) {
    const bool down = sample.*binding.down;
    // branch-gate: BG-1037
    if (action == InputAction::None && down && !(state.*binding.wasDown)) {
      action = actionForInput(binding.input);
    }
  }
  for (const KeyboardMenuActionBinding& binding : kKeyboardMenuActionBindings) {
    state.*binding.wasDown = sample.*binding.down;
  }
  return action;
}

InputAction pollKeyboardMenuAction(KeyboardInputState& state) {
#if defined(IGGY3D_HAS_SDL3)
  const bool* keys = SDL_GetKeyboardState(nullptr);
  KeyboardMenuInputSample sample;
  sample.upDown = keyDown(keys, SDL_SCANCODE_UP) || keyDown(keys, SDL_SCANCODE_W);
  sample.downDown = keyDown(keys, SDL_SCANCODE_DOWN) || keyDown(keys, SDL_SCANCODE_S);
  sample.leftDown = keyDown(keys, SDL_SCANCODE_LEFT) || keyDown(keys, SDL_SCANCODE_A);
  sample.rightDown = keyDown(keys, SDL_SCANCODE_RIGHT) || keyDown(keys, SDL_SCANCODE_D);
  sample.confirmDown = keyDown(keys, SDL_SCANCODE_RETURN) || keyDown(keys, SDL_SCANCODE_SPACE);
  sample.backDown = keyDown(keys, SDL_SCANCODE_ESCAPE);
  sample.tabDown = keyDown(keys, SDL_SCANCODE_TAB);
  sample.devToggleDown = keyDown(keys, SDL_SCANCODE_F1);
  sample.devCollisionOverlayDown = keyDown(keys, SDL_SCANCODE_F2);
  sample.debugOverlayDown = keyDown(keys, SDL_SCANCODE_F3);
  sample.movementTuningToggleDown = keyDown(keys, SDL_SCANCODE_F4);
  sample.mapMakerToggleDown = keyDown(keys, SDL_SCANCODE_M);
  return recordKeyboardMenuAction(state, sample);
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
  constexpr std::array<SDL_Scancode, kKeyboardAsciiRoomPaintGlyphCount> kScanCodes = {
      SDL_SCANCODE_1,
      SDL_SCANCODE_2,
      SDL_SCANCODE_3,
      SDL_SCANCODE_4,
      SDL_SCANCODE_5,
      SDL_SCANCODE_6,
      SDL_SCANCODE_7,
      SDL_SCANCODE_8,
      SDL_SCANCODE_9,
      SDL_SCANCODE_0,
      SDL_SCANCODE_MINUS,
      SDL_SCANCODE_EQUALS,
      SDL_SCANCODE_R,
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
  const bool jumpDown = keyDown(keys, SDL_SCANCODE_SPACE);
  const bool dashDown =
      keyDown(keys, SDL_SCANCODE_LCTRL) || keyDown(keys, SDL_SCANCODE_RCTRL);
  const bool sprintDown =
      keyDown(keys, SDL_SCANCODE_LSHIFT) || keyDown(keys, SDL_SCANCODE_RSHIFT);

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
  // branch-gate: BG-1150
  if (sprintDown) {
    recordAction(actions, InputAction::PlayerSprint, true, false, false, 1.0F);
  }
  // branch-gate: BG-1152
  if (jumpDown) {
    recordAction(actions, InputAction::PlayerJump, true, !state.jumpWasDown, false, 1.0F);
  }
  // branch-gate: BG-1154
  if (dashDown && !state.dashWasDown) {
    recordAction(actions, InputAction::PlayerDash, true, true, false, 1.0F);
  }
  // branch-gate: BG-1205
  if (dashDown) {
    recordAction(actions, InputAction::PlayerCrouch, true, false, false, 1.0F);
  }
  if (interactDown && !state.interactWasDown) {
    recordAction(actions, InputAction::PlayerInteract, true, true, false, 1.0F);
  }
  if (retryDown && !state.retryWasDown) {
    recordAction(actions, InputAction::PlayerRetryOrReset, true, true, false, 1.0F);
  }

  state.jumpWasDown = jumpDown;
  state.dashWasDown = dashDown;
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
