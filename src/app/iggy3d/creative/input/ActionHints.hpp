#pragma once

#include "app/iggy3d/creative/input/ControlProfile.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string_view>

namespace iggy3d::creative {

inline constexpr std::size_t kCreativeActionHintCapacity = 6U;
inline constexpr std::size_t kCreativeActionHintActionCapacity = 2U;
inline constexpr std::size_t kCreativeActionHintTextCapacity = 48U;
inline constexpr std::uint16_t kCreativeActionHintAnyGroupOrdinal =
    std::numeric_limits<std::uint16_t>::max();

struct CreativeActionHintFixedText {
  std::array<char, kCreativeActionHintTextCapacity> bytes{};
  std::uint8_t length = 0U;

  [[nodiscard]] std::string_view view() const noexcept {
    return {bytes.data(), length};
  }
};

struct CreativeActionHintSpec {
  std::array<CreativeInputActionId, kCreativeActionHintActionCapacity>
      actions{CreativeInputActionId::Count, CreativeInputActionId::Count};
  std::array<std::uint16_t, kCreativeActionHintActionCapacity>
      preferredGroupOrdinals{kCreativeActionHintAnyGroupOrdinal,
                             kCreativeActionHintAnyGroupOrdinal};
  std::uint8_t actionCount = 0U;
  std::string_view label;
};

struct CreativeActionHint {
  std::array<CreativeInputActionId, kCreativeActionHintActionCapacity>
      actions{CreativeInputActionId::Count, CreativeInputActionId::Count};
  std::array<CreativeInputKey, kCreativeActionHintActionCapacity>
      triggers{CreativeInputKey::Unbound, CreativeInputKey::Unbound};
  std::uint8_t actionCount = 0U;
  CreativeActionHintFixedText chord;
  CreativeActionHintFixedText label;
};

struct CreativeActionHintFrame {
  std::array<CreativeActionHint, kCreativeActionHintCapacity> hints{};
  std::uint8_t count = 0U;
  std::uint8_t unresolvedCount = 0U;
  bool capacityExceeded = false;
  bool invalidInput = false;
  bool textTruncated = false;

  [[nodiscard]] std::span<const CreativeActionHint> items() const noexcept {
    return {hints.data(), count};
  }
};

struct CreativeControlDeviceActivity {
  bool keyboardMouse = false;
  bool gamepad = false;
};

[[nodiscard]] CreativeControlDeviceActivity
measureCreativeControlDeviceActivity(
    const CreativeInputFrame& frame,
    bool pointerActivity,
    bool gamepadAnalogActivity) noexcept;

[[nodiscard]] CreativeControlDevice resolveCreativeActiveControlDevice(
    CreativeControlDevice previous,
    CreativeControlDeviceActivity activity) noexcept;

// O(specs * actions-per-hint * profile bindings), bounded by 6 * 2 * 168.
[[nodiscard]] CreativeActionHintFrame resolveCreativeActionHints(
    const CreativeControlProfile& profile,
    CreativeInputContext context,
    CreativeControlDevice device,
    CreativeInputPlatform platform,
    std::span<const CreativeActionHintSpec> specs) noexcept;

}  // namespace iggy3d::creative
