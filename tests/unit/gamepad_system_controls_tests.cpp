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

bool optionsOpensDevMenu() {
  iggy3d::GamepadSystemControlContext context;
  context.devMenuEnabled = true;
  iggy3d::GamepadSystemControlState state;
  iggy3d::GamepadSystemControlSample sample;
  sample.optionsDown = true;
  const iggy3d::GamepadSystemControlResult result =
      iggy3d::mapGamepadSystemControls(context, sample, state);
  return expect(result.devToggleRequested, "options toggles dev menu") &&
         expect(!result.quitRequested, "options alone does not quit") &&
         expect(result.consumeOptions, "options consumed") &&
         expect(result.actionButton == "options", "options action name");
}

bool createOptionsQuitsWithoutOpeningDevMenu() {
  iggy3d::GamepadSystemControlContext context;
  context.devMenuEnabled = true;
  iggy3d::GamepadSystemControlState state;
  iggy3d::GamepadSystemControlSample sample;
  sample.optionsDown = true;
  sample.createDown = true;
  const iggy3d::GamepadSystemControlResult result =
      iggy3d::mapGamepadSystemControls(context, sample, state);
  return expect(result.quitRequested, "create options quits") &&
         expect(!result.devToggleRequested, "quit combo does not toggle dev menu") &&
         expect(result.consumeCreate && result.consumeOptions, "quit combo consumed") &&
         expect(result.actionButton == "create+options", "quit action name");
}

bool holdingOptionsDoesNotRetoggleEveryFrame() {
  iggy3d::GamepadSystemControlContext context;
  context.devMenuEnabled = true;
  iggy3d::GamepadSystemControlState state;
  iggy3d::GamepadSystemControlSample sample;
  sample.optionsDown = true;
  const iggy3d::GamepadSystemControlResult first =
      iggy3d::mapGamepadSystemControls(context, sample, state);
  const iggy3d::GamepadSystemControlResult second =
      iggy3d::mapGamepadSystemControls(context, sample, state);
  return expect(first.devToggleRequested, "first options press toggles") &&
         expect(!second.devToggleRequested, "held options does not retoggle") &&
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
  context.devMenuEnabled = true;
  context.editorOpen = true;
  iggy3d::GamepadSystemControlState state;
  iggy3d::GamepadSystemControlSample sample;
  sample.optionsDown = true;
  const iggy3d::GamepadSystemControlResult result =
      iggy3d::mapGamepadSystemControls(context, sample, state);
  return expect(!result.devToggleRequested, "options ignored while editor open") &&
         expect(!result.quitRequested, "options still does not quit while editor open");
}

}  // namespace

int main() {
  const bool ok = optionsOpensDevMenu() && createOptionsQuitsWithoutOpeningDevMenu() &&
                  holdingOptionsDoesNotRetoggleEveryFrame() && createEastTogglesEditor() &&
                  optionsIgnoredWhenEditorIsOpen();
  return ok ? 0 : 1;
}
