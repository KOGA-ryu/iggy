#pragma once

#include <array>

#include "app/input/InputAction.hpp"

namespace iggy3d {

struct ActionState;

struct KeyboardInputState {
  bool upWasDown = false;
  bool downWasDown = false;
  bool leftWasDown = false;
  bool rightWasDown = false;
  bool confirmWasDown = false;
  bool backWasDown = false;
  bool tabWasDown = false;
  std::array<bool, 7> asciiPaintWasDown{};
  bool interactWasDown = false;
  bool retryWasDown = false;
  bool editorUpWasDown = false;
  bool editorDownWasDown = false;
  bool editorLeftWasDown = false;
  bool editorRightWasDown = false;
  bool editorNextToolWasDown = false;
  bool editorPreviousToolWasDown = false;
  bool editorPlaceWasDown = false;
};

struct KeyboardRoomEditorInputSample {
  bool upDown = false;
  bool downDown = false;
  bool leftDown = false;
  bool rightDown = false;
  bool nextToolDown = false;
  bool previousToolDown = false;
  bool placeDown = false;
};

InputAction pollKeyboardMenuAction(KeyboardInputState& state);
char pollKeyboardAsciiRoomPaintGlyph(KeyboardInputState& state);
void pollKeyboardGameplayActions(KeyboardInputState& state, ActionState& actions);
void recordKeyboardRoomEditorActions(KeyboardInputState& state,
                                     const KeyboardRoomEditorInputSample& sample,
                                     ActionState& actions);
void pollKeyboardRoomEditorActions(KeyboardInputState& state, ActionState& actions);

}  // namespace iggy3d
