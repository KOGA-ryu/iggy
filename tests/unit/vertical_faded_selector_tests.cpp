#include "app/frontend/VerticalFadedSelector.hpp"

#include <iostream>
#include <string_view>
#include <vector>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

std::vector<iggy3d::VerticalSelectorItem> makeItems() {
  return {
      {.id = "continue", .title = "Continue"},
      {.id = "new_world", .title = "New World"},
      {.id = "load_save", .title = "Load Save"},
      {.id = "settings", .title = "Settings"},
      {.id = "dev_tools", .title = "Dev Tools"},
      {.id = "exit", .title = "Exit"},
  };
}

bool defaultConfigMatchesContract() {
  const iggy3d::VerticalSelectorConfig config;
  return expect(config.visibleEntryCount == 5, "visible count") &&
         expect(!config.wraparound, "no wrap") &&
         expect(config.snapToEntries, "snap enabled") &&
         expect(config.topFadeZoneFraction == 0.2F, "top fade") &&
         expect(config.bottomFadeZoneFraction == 0.2F, "bottom fade") &&
         expect(config.heldRepeatDelayMs == 300, "repeat delay");
}

bool emptyListIsSafe() {
  const std::vector<iggy3d::VerticalSelectorItem> items;
  const auto state = iggy3d::makeVerticalSelectorState(items.size());
  const auto result = iggy3d::applyVerticalSelectorInput(
      items, state, iggy3d::VerticalSelectorInput::Confirm);
  return expect(result.selectedId == "none", "empty selected id") &&
         expect(!result.confirmed, "empty confirm ignored") &&
         expect(result.status == "selector_empty", "empty status");
}

bool singleItemClamps() {
  const std::vector<iggy3d::VerticalSelectorItem> items = {
      {.id = "only", .title = "Only"},
  };
  auto state = iggy3d::makeVerticalSelectorState(items.size());
  auto result = iggy3d::applyVerticalSelectorInput(
      items, state, iggy3d::VerticalSelectorInput::Down);
  bool ok = expect(result.state.selectedIndex == 0, "single down clamps") &&
            expect(result.selectedId == "only", "single down id");
  state = result.state;
  result = iggy3d::applyVerticalSelectorInput(
      items, state, iggy3d::VerticalSelectorInput::Up);
  ok = expect(result.state.selectedIndex == 0, "single up clamps") &&
       expect(result.selectedId == "only", "single up id") && ok;
  return ok;
}

bool multiItemNavigationClampsAndTracksIds() {
  const auto items = makeItems();
  auto state = iggy3d::makeVerticalSelectorState(items.size(), 0);
  auto result = iggy3d::applyVerticalSelectorInput(
      items, state, iggy3d::VerticalSelectorInput::Up);
  bool ok = expect(result.state.selectedIndex == 0, "top clamps") &&
            expect(result.status == "selector_clamped", "top status") &&
            expect(result.selectedId == "continue", "top id");
  state = result.state;
  result = iggy3d::applyVerticalSelectorInput(
      items, state, iggy3d::VerticalSelectorInput::Down);
  ok = expect(result.state.selectedIndex == 1, "down moves") &&
       expect(result.scrollDirection == "down", "down direction") &&
       expect(result.selectedId == "new_world", "down id") && ok;
  state = iggy3d::makeVerticalSelectorState(items.size(), items.size() - 1);
  result = iggy3d::applyVerticalSelectorInput(
      items, state, iggy3d::VerticalSelectorInput::Down);
  ok = expect(result.state.selectedIndex == items.size() - 1, "bottom clamps") &&
       expect(result.selectedId == "exit", "bottom id") && ok;
  return ok;
}

bool disabledFocusedItemCannotConfirm() {
  std::vector<iggy3d::VerticalSelectorItem> items = makeItems();
  items[2].enabled = false;
  items[2].disabledReason = "no_compatible_save";
  const auto state = iggy3d::makeVerticalSelectorState(items.size(), 2);
  const auto result = iggy3d::applyVerticalSelectorInput(
      items, state, iggy3d::VerticalSelectorInput::Confirm);
  return expect(result.selectedId == "load_save", "disabled selected id") &&
         expect(!result.confirmed, "disabled not confirmed") &&
         expect(result.status == "no_compatible_save", "disabled reason status");
}

bool enabledItemConfirmsAndBackReports() {
  const auto items = makeItems();
  const auto state = iggy3d::makeVerticalSelectorState(items.size(), 3);
  auto result = iggy3d::applyVerticalSelectorInput(
      items, state, iggy3d::VerticalSelectorInput::Confirm);
  bool ok = expect(result.confirmed, "enabled confirms") &&
            expect(result.selectedId == "settings", "confirmed id") &&
            expect(result.status == "selector_confirmed", "confirmed status");
  result = iggy3d::applyVerticalSelectorInput(
      items, state, iggy3d::VerticalSelectorInput::Back);
  ok = expect(result.backRequested, "back requested") &&
       expect(!result.confirmed, "back does not confirm") &&
       expect(result.status == "selector_back_requested", "back status") && ok;
  return ok;
}

bool pageNavigationClampsLocally() {
  const auto items = makeItems();
  const iggy3d::VerticalSelectorConfig config;
  auto state = iggy3d::makeVerticalSelectorState(items.size(), 0);
  auto result = iggy3d::applyVerticalSelectorInput(
      items, state, iggy3d::VerticalSelectorInput::PageDown, config);
  bool ok = expect(result.state.selectedIndex == 5, "page down moves by visible page") &&
            expect(result.selectedId == "exit", "page down id");
  state = result.state;
  result = iggy3d::applyVerticalSelectorInput(
      items, state, iggy3d::VerticalSelectorInput::PageDown, config);
  ok = expect(result.state.selectedIndex == 5, "page down bottom clamps") &&
       expect(result.status == "selector_clamped", "page down clamp status") && ok;
  result = iggy3d::applyVerticalSelectorInput(
      items, state, iggy3d::VerticalSelectorInput::PageUp, config);
  ok = expect(result.state.selectedIndex == 0, "page up clamps to top") &&
       expect(result.selectedId == "continue", "page up id") && ok;
  return ok;
}

}  // namespace

int main() {
  const bool ok = defaultConfigMatchesContract() && emptyListIsSafe() &&
                  singleItemClamps() && multiItemNavigationClampsAndTracksIds() &&
                  disabledFocusedItemCannotConfirm() &&
                  enabledItemConfirmsAndBackReports() &&
                  pageNavigationClampsLocally();
  return ok ? 0 : 1;
}
