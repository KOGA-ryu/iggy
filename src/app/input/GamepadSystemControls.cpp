#include "app/input/GamepadSystemControls.hpp"

namespace iggy3d {

GamepadSystemControlResult mapGamepadSystemControls(
    const GamepadSystemControlContext& context,
    const GamepadSystemControlSample& sample,
    GamepadSystemControlState& state) {
  const bool optionsPressed = sample.optionsDown && !state.optionsWasDown;
  const bool createPressed = sample.createDown && !state.createWasDown;

  GamepadSystemControlResult result;
  const bool quitComboPressed =
      sample.optionsDown && sample.createDown && (optionsPressed || createPressed);
  if (quitComboPressed) {
    result.quitRequested = true;
    result.consumeOptions = true;
    result.consumeCreate = true;
    result.actionButton = "create+options";
  } else if (context.editorEnabled && sample.createDown && sample.eastDown) {
    result.editorToggleRequested = true;
    result.consumeCreate = true;
    result.consumeEast = true;
    result.actionButton = "create+east";
  } else if (context.devMenuEnabled && !context.editorOpen && optionsPressed) {
    result.devToggleRequested = true;
    result.consumeOptions = true;
    result.actionButton = "options";
  }

  state.optionsWasDown = sample.optionsDown;
  state.createWasDown = sample.createDown;
  return result;
}

}  // namespace iggy3d
