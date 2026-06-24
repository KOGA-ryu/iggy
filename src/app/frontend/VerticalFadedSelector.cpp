#include "app/frontend/VerticalFadedSelector.hpp"

#include <algorithm>

namespace iggy3d {
namespace {

std::size_t clampSelectedIndex(std::size_t itemCount, std::size_t selectedIndex) {
  if (itemCount == 0) {
    return 0;
  }
  return std::min(selectedIndex, itemCount - 1);
}

std::size_t pageStep(const VerticalSelectorConfig& config) {
  return std::max<std::size_t>(config.visibleEntryCount, 1);
}

VerticalSelectorResult baseResult(const std::vector<VerticalSelectorItem>& items,
                                  VerticalSelectorState state) {
  state.selectedIndex = clampSelectedIndex(items.size(), state.selectedIndex);
  VerticalSelectorResult result;
  result.state = state;
  result.selectedId = selectedVerticalSelectorId(items, state);
  return result;
}

}  // namespace

std::string_view verticalSelectorInputName(VerticalSelectorInput input) {
  switch (input) {
    case VerticalSelectorInput::None:
      return "none";
    case VerticalSelectorInput::Up:
      return "up";
    case VerticalSelectorInput::Down:
      return "down";
    case VerticalSelectorInput::PageUp:
      return "page_up";
    case VerticalSelectorInput::PageDown:
      return "page_down";
    case VerticalSelectorInput::Confirm:
      return "confirm";
    case VerticalSelectorInput::Back:
      return "back";
  }
  return "unknown";
}

VerticalSelectorState makeVerticalSelectorState(std::size_t itemCount,
                                                std::size_t selectedIndex,
                                                bool focused) {
  VerticalSelectorState state;
  state.selectedIndex = clampSelectedIndex(itemCount, selectedIndex);
  state.focused = focused;
  return state;
}

std::string selectedVerticalSelectorId(const std::vector<VerticalSelectorItem>& items,
                                       const VerticalSelectorState& state) {
  if (items.empty() || state.selectedIndex >= items.size()) {
    return "none";
  }
  return items[state.selectedIndex].id.empty() ? "none" : items[state.selectedIndex].id;
}

VerticalSelectorResult applyVerticalSelectorInput(
    const std::vector<VerticalSelectorItem>& items,
    const VerticalSelectorState& state,
    VerticalSelectorInput input,
    const VerticalSelectorConfig& config) {
  VerticalSelectorResult result = baseResult(items, state);

  if (input == VerticalSelectorInput::Back) {
    result.backRequested = true;
    result.status = "selector_back_requested";
    return result;
  }

  if (items.empty()) {
    result.status = "selector_empty";
    return result;
  }

  const auto lastIndex = items.size() - 1;
  switch (input) {
    case VerticalSelectorInput::None:
      result.status = "selector_ready";
      break;
    case VerticalSelectorInput::Up:
      if (result.state.selectedIndex == 0) {
        result.status = "selector_clamped";
      } else {
        --result.state.selectedIndex;
        result.scrollDirection = "up";
        result.status = "selector_moved";
      }
      break;
    case VerticalSelectorInput::Down:
      if (result.state.selectedIndex >= lastIndex) {
        result.status = "selector_clamped";
      } else {
        ++result.state.selectedIndex;
        result.scrollDirection = "down";
        result.status = "selector_moved";
      }
      break;
    case VerticalSelectorInput::PageUp: {
      const auto oldIndex = result.state.selectedIndex;
      result.state.selectedIndex =
          (oldIndex > pageStep(config)) ? oldIndex - pageStep(config) : 0;
      if (result.state.selectedIndex == oldIndex) {
        result.status = "selector_clamped";
      } else {
        result.scrollDirection = "up";
        result.status = "selector_moved";
      }
      break;
    }
    case VerticalSelectorInput::PageDown: {
      const auto oldIndex = result.state.selectedIndex;
      result.state.selectedIndex = std::min(lastIndex, oldIndex + pageStep(config));
      if (result.state.selectedIndex == oldIndex) {
        result.status = "selector_clamped";
      } else {
        result.scrollDirection = "down";
        result.status = "selector_moved";
      }
      break;
    }
    case VerticalSelectorInput::Confirm: {
      const auto& selected = items[result.state.selectedIndex];
      if (!selected.enabled) {
        result.status = selected.disabledReason.empty() ? "selector_disabled"
                                                       : selected.disabledReason;
      } else {
        result.confirmed = true;
        result.status = "selector_confirmed";
      }
      break;
    }
    case VerticalSelectorInput::Back:
      break;
  }

  result.selectedId = selectedVerticalSelectorId(items, result.state);
  return result;
}

}  // namespace iggy3d
