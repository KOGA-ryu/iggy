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
  bool devToggleWasDown = false;
  std::array<bool, 8> asciiPaintWasDown{};
  bool jumpWasDown = false;
  bool dashWasDown = false;
  bool interactWasDown = false;
  bool retryWasDown = false;
  bool editorUpWasDown = false;
  bool editorDownWasDown = false;
  bool editorLeftWasDown = false;
  bool editorRightWasDown = false;
  bool editorNextToolWasDown = false;
  bool editorPreviousToolWasDown = false;
  bool editorSelectFloorToolWasDown = false;
  bool editorSelectWallToolWasDown = false;
  bool editorRotateWallDirectionWasDown = false;
  bool editorPreviewPlacementWasDown = false;
  bool editorConfirmPreviewWasDown = false;
  bool editorCancelPreviewWasDown = false;
  bool editorPlaceWasDown = false;
  bool editorDeleteWasDown = false;
  bool editorUndoWasDown = false;
  bool editorRedoWasDown = false;
};

struct KeyboardRoomEditorInputSample {
  bool upDown = false;
  bool downDown = false;
  bool leftDown = false;
  bool rightDown = false;
  bool nextToolDown = false;
  bool previousToolDown = false;
  bool selectFloorToolDown = false;
  bool selectWallToolDown = false;
  bool rotateWallDirectionDown = false;
  bool previewPlacementDown = false;
  bool confirmPreviewDown = false;
  bool cancelPreviewDown = false;
  bool placeDown = false;
  bool deleteDown = false;
  bool undoDown = false;
  bool redoDown = false;
};

struct KeyboardAsciiRoomPaintSample {
  std::array<bool, 8> glyphDown{};
};

InputAction pollKeyboardMenuAction(KeyboardInputState& state);
char recordKeyboardAsciiRoomPaintGlyph(KeyboardInputState& state,
                                       const KeyboardAsciiRoomPaintSample& sample);
char pollKeyboardAsciiRoomPaintGlyph(KeyboardInputState& state);
void pollKeyboardGameplayActions(KeyboardInputState& state, ActionState& actions);
void recordKeyboardRoomEditorActions(KeyboardInputState& state,
                                     const KeyboardRoomEditorInputSample& sample,
                                     ActionState& actions);
void pollKeyboardRoomEditorActions(KeyboardInputState& state, ActionState& actions);

}  // namespace iggy3d
