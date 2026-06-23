#pragma once

#include <string_view>

namespace iggy3d {

struct GamepadSystemControlContext {
  bool devMenuEnabled = false;
  bool pauseMenuEnabled = false;
  bool devOverlayDirectEnabled = false;
  bool editorEnabled = false;
  bool editorOpen = false;
  bool menuOwnsInput = false;
};

struct GamepadSystemControlSample {
  bool optionsDown = false;
  bool createDown = false;
  bool eastDown = false;
};

struct GamepadSystemControlState {
  bool optionsWasDown = false;
  bool createWasDown = false;
};

struct GamepadSystemControlResult {
  bool devToggleRequested = false;
  bool pauseToggleRequested = false;
  bool editorToggleRequested = false;
  bool quitRequested = false;
  bool consumeOptions = false;
  bool consumeCreate = false;
  bool consumeEast = false;
  std::string_view actionButton = "unavailable";
};

GamepadSystemControlResult mapGamepadSystemControls(
    const GamepadSystemControlContext& context,
    const GamepadSystemControlSample& sample,
    GamepadSystemControlState& state);

}  // namespace iggy3d
