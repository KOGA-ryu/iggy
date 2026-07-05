#pragma once

#include <array>
#include <cstddef>

#include "app/input/InputAction.hpp"

namespace iggy3d {

struct ActionState;

inline constexpr std::size_t kKeyboardAsciiRoomPaintGlyphCount = 13U;

struct KeyboardInputState {
  bool upWasDown = false;
  bool downWasDown = false;
  bool leftWasDown = false;
  bool rightWasDown = false;
  bool confirmWasDown = false;
  bool backWasDown = false;
  bool tabWasDown = false;
  bool devToggleWasDown = false;
  bool devCollisionOverlayWasDown = false;
  bool debugOverlayWasDown = false;
  bool movementTuningToggleWasDown = false;
  bool mapMakerToggleWasDown = false;
  std::array<bool, kKeyboardAsciiRoomPaintGlyphCount> asciiPaintWasDown{};
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
  bool creativeToolSelectWasDown = false;
  bool creativeToolMoveWasDown = false;
  bool creativeToolMeasureWasDown = false;
  bool creativeToolNavigateWasDown = false;
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
  std::array<bool, kKeyboardAsciiRoomPaintGlyphCount> glyphDown{};
};

// Creative document editor tool keys (1/2/3/4). Deliberately narrow: this is
// the ONLY keyboard surface in creative document mode and it never records
// gameplay actions.
struct KeyboardCreativeToolInputSample {
  bool selectToolDown = false;
  bool moveToolDown = false;
  bool measureToolDown = false;
  bool navigateToolDown = false;
};

struct KeyboardCreativeToolKeyPresses {
  bool selectPressed = false;
  bool movePressed = false;
  bool measurePressed = false;
  bool navigatePressed = false;
};

struct KeyboardMenuInputSample {
  bool debugOverlayDown = false;
  bool movementTuningToggleDown = false;
  bool devToggleDown = false;
  bool devCollisionOverlayDown = false;
  bool mapMakerToggleDown = false;
  bool upDown = false;
  bool downDown = false;
  bool leftDown = false;
  bool rightDown = false;
  bool confirmDown = false;
  bool backDown = false;
  bool tabDown = false;
};

InputAction recordKeyboardMenuAction(KeyboardInputState& state,
                                     const KeyboardMenuInputSample& sample);
InputAction pollKeyboardMenuAction(KeyboardInputState& state);
char recordKeyboardAsciiRoomPaintGlyph(KeyboardInputState& state,
                                       const KeyboardAsciiRoomPaintSample& sample);
char pollKeyboardAsciiRoomPaintGlyph(KeyboardInputState& state);
void pollKeyboardGameplayActions(KeyboardInputState& state, ActionState& actions);
void recordKeyboardRoomEditorActions(KeyboardInputState& state,
                                     const KeyboardRoomEditorInputSample& sample,
                                     ActionState& actions);
void pollKeyboardRoomEditorActions(KeyboardInputState& state, ActionState& actions);
KeyboardCreativeToolKeyPresses recordKeyboardCreativeToolKeys(
    KeyboardInputState& state,
    const KeyboardCreativeToolInputSample& sample);
KeyboardCreativeToolKeyPresses pollKeyboardCreativeToolKeys(
    KeyboardInputState& state);

}  // namespace iggy3d
