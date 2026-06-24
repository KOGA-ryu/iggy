#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d {

struct VerticalSelectorItem {
  std::string id;
  std::string kind;
  std::string title;
  std::string subtitle;
  std::string snapshotRef;
  std::string timestamp;
  bool enabled = true;
  std::string disabledReason = "none";
};

struct VerticalSelectorState {
  std::size_t selectedIndex = 0;
  bool focused = false;
};

struct VerticalSelectorConfig {
  std::size_t visibleEntryCount = 5;
  bool wraparound = false;
  bool snapToEntries = true;
  float topFadeZoneFraction = 0.2F;
  float bottomFadeZoneFraction = 0.2F;
  std::uint32_t heldRepeatDelayMs = 300;
};

enum class VerticalSelectorInput : std::uint8_t {
  None,
  Up,
  Down,
  PageUp,
  PageDown,
  Confirm,
  Back,
};

struct VerticalSelectorResult {
  VerticalSelectorState state;
  std::string selectedId = "none";
  std::string scrollDirection = "none";
  bool confirmed = false;
  bool backRequested = false;
  std::string status = "selector_ready";
};

std::string_view verticalSelectorInputName(VerticalSelectorInput input);

VerticalSelectorState makeVerticalSelectorState(std::size_t itemCount,
                                                std::size_t selectedIndex = 0,
                                                bool focused = true);

std::string selectedVerticalSelectorId(const std::vector<VerticalSelectorItem>& items,
                                       const VerticalSelectorState& state);

VerticalSelectorResult applyVerticalSelectorInput(
    const std::vector<VerticalSelectorItem>& items,
    const VerticalSelectorState& state,
    VerticalSelectorInput input,
    const VerticalSelectorConfig& config = VerticalSelectorConfig{});

}  // namespace iggy3d
