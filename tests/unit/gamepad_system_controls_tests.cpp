#include "app/input/GamepadSystemControls.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool optionsOpensPauseMenu() {
  iggy3d::GamepadSystemControlContext context;
  context.pauseMenuEnabled = true;
  iggy3d::GamepadSystemControlState state;
  iggy3d::GamepadSystemControlSample sample;
  sample.optionsDown = true;
  const iggy3d::GamepadSystemControlResult result =
      iggy3d::mapGamepadSystemControls(context, sample, state);
  return expect(result.pauseToggleRequested, "options toggles pause menu") &&
         expect(!result.devToggleRequested, "options does not toggle dev menu") &&
         expect(!result.quitRequested, "options alone does not quit") &&
         expect(result.consumeOptions, "options consumed") &&
         expect(result.actionButton == "options", "options action name");
}

bool createOptionsQuitsWithoutOpeningDevMenu() {
  iggy3d::GamepadSystemControlContext context;
  context.pauseMenuEnabled = true;
  context.devMenuEnabled = true;
  iggy3d::GamepadSystemControlState state;
  iggy3d::GamepadSystemControlSample sample;
  sample.optionsDown = true;
  sample.createDown = true;
  const iggy3d::GamepadSystemControlResult result =
      iggy3d::mapGamepadSystemControls(context, sample, state);
  return expect(result.quitRequested, "create options quits") &&
         expect(!result.devToggleRequested, "quit combo does not toggle dev menu") &&
         expect(!result.pauseToggleRequested, "quit combo does not toggle pause menu") &&
         expect(result.consumeCreate && result.consumeOptions, "quit combo consumed") &&
         expect(result.actionButton == "create+options", "quit action name");
}

bool holdingOptionsDoesNotRetoggleEveryFrame() {
  iggy3d::GamepadSystemControlContext context;
  context.pauseMenuEnabled = true;
  iggy3d::GamepadSystemControlState state;
  iggy3d::GamepadSystemControlSample sample;
  sample.optionsDown = true;
  const iggy3d::GamepadSystemControlResult first =
      iggy3d::mapGamepadSystemControls(context, sample, state);
  const iggy3d::GamepadSystemControlResult second =
      iggy3d::mapGamepadSystemControls(context, sample, state);
  return expect(first.pauseToggleRequested, "first options press toggles pause") &&
         expect(!second.pauseToggleRequested, "held options does not retoggle pause") &&
         expect(!second.devToggleRequested, "held options does not toggle dev") &&
         expect(!second.quitRequested, "held options does not quit");
}

bool createEastTogglesEditor() {
  iggy3d::GamepadSystemControlContext context;
  context.devMenuEnabled = true;
  context.editorEnabled = true;
  iggy3d::GamepadSystemControlState state;
  iggy3d::GamepadSystemControlSample sample;
  sample.createDown = true;
  sample.eastDown = true;
  const iggy3d::GamepadSystemControlResult result =
      iggy3d::mapGamepadSystemControls(context, sample, state);
  return expect(result.editorToggleRequested, "create east toggles editor") &&
         expect(!result.devToggleRequested, "editor combo does not toggle dev") &&
         expect(!result.quitRequested, "editor combo does not quit") &&
         expect(result.consumeCreate && result.consumeEast, "editor combo consumed");
}

bool optionsIgnoredWhenEditorIsOpen() {
  iggy3d::GamepadSystemControlContext context;
  context.pauseMenuEnabled = true;
  context.devMenuEnabled = true;
  context.editorOpen = true;
  iggy3d::GamepadSystemControlState state;
  iggy3d::GamepadSystemControlSample sample;
  sample.optionsDown = true;
  const iggy3d::GamepadSystemControlResult result =
      iggy3d::mapGamepadSystemControls(context, sample, state);
  return expect(!result.devToggleRequested, "options ignored while editor open") &&
         expect(!result.pauseToggleRequested, "options does not pause while editor open") &&
         expect(!result.quitRequested, "options still does not quit while editor open");
}

bool directDevRequiresExplicitContext() {
  iggy3d::GamepadSystemControlContext context;
  context.devMenuEnabled = true;
  iggy3d::GamepadSystemControlState state;
  iggy3d::GamepadSystemControlSample sample;
  sample.optionsDown = true;
  const iggy3d::GamepadSystemControlResult disabled =
      iggy3d::mapGamepadSystemControls(context, sample, state);
  state = {};
  context.devOverlayDirectEnabled = true;
  const iggy3d::GamepadSystemControlResult enabled =
      iggy3d::mapGamepadSystemControls(context, sample, state);
  return expect(!disabled.devToggleRequested, "direct dev disabled by default") &&
         expect(enabled.devToggleRequested, "direct dev requires opt-in") &&
         expect(enabled.consumeOptions, "direct dev consumes options");
}

}  // namespace

int main() {
  const bool ok = optionsOpensPauseMenu() && createOptionsQuitsWithoutOpeningDevMenu() &&
                  holdingOptionsDoesNotRetoggleEveryFrame() && createEastTogglesEditor() &&
                  optionsIgnoredWhenEditorIsOpen() && directDevRequiresExplicitContext();
  return ok ? 0 : 1;
}
