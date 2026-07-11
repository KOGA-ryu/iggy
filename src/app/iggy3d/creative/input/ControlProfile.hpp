#pragma once

#include "app/iggy3d/creative/input/ControllerInput.hpp"
#include "app/iggy3d/creative/input/InputRouter.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace iggy3d::creative {

enum class CreativeControlDevice : std::uint8_t {
  KeyboardMouse,
  Gamepad,
  Count,
};

enum class CreativeControlConflictPolicy : std::uint8_t {
  Reject,
  Replace,
  Swap,
  Count,
};

enum class CreativeControlRebindStatus : std::uint8_t {
  Applied,
  NoChange,
  Conflict,
  MultipleConflicts,
  InvalidGroup,
  ReservedAction,
  InvalidKey,
  DeviceMismatch,
  InvalidProfile,
  Count,
};

enum class CreativeControlSettingId : std::uint8_t {
  MouseLookSensitivity,
  GamepadLookSensitivity,
  MovementDeadzone,
  MovementResponse,
  LookDeadzone,
  LookResponse,
  InvertLookX,
  InvertLookY,
  MenuRepeatDelay,
  MenuRepeatInterval,
  Count,
};

inline constexpr std::size_t kCreativeControlSettingCount =
    static_cast<std::size_t>(CreativeControlSettingId::Count);

struct CreativeControlProfile {
  std::array<CreativeInputBinding, kCreativeInputBindingCapacity> bindings{};
  std::array<std::uint16_t, kCreativeInputBindingCapacity> bindingGroups{};
  std::array<CreativeInputActionId, kCreativeInputBindingCapacity>
      groupActions{};
  std::array<CreativeControlDevice, kCreativeInputBindingCapacity>
      groupDevices{};
  std::array<std::uint16_t, kCreativeInputBindingCapacity> groupOrdinals{};
  std::size_t bindingCount = 0;
  std::size_t groupCount = 0;

  CreativeStickProfile movementStick{};
  CreativeStickProfile lookStick{};
  float mouseLookSensitivity = 0.12F;
  float gamepadLookSensitivity = 2.4F;
  std::uint16_t menuRepeatDelayMilliseconds = 320;
  std::uint16_t menuRepeatIntervalMilliseconds = 110;

  [[nodiscard]] std::span<const CreativeInputBinding>
  bindingSpan() const noexcept {
    return {bindings.data(), bindingCount};
  }

  [[nodiscard]] std::span<CreativeInputBinding> bindingSpan() noexcept {
    return {bindings.data(), bindingCount};
  }
};

struct CreativeControlBindingRow {
  std::uint16_t group = 0;
  std::uint16_t ordinal = 0;
  CreativeInputActionId action = CreativeInputActionId::HotbarSlot1;
  CreativeInputKey trigger = CreativeInputKey::Unbound;
  CreativeInputModifierMask requiredAllModifiers =
      kCreativeInputModifierNone;
  CreativeInputModifierMask requiredAnyModifiers =
      kCreativeInputModifierNone;
  CreativeInputModifierMask allowedModifiers = kCreativeInputModifierNone;
  CreativeControlDevice device = CreativeControlDevice::KeyboardMouse;
  CreativeInputBindingActivation activation =
      CreativeInputBindingActivation::Press;
};

struct CreativeControlBindingList {
  std::array<CreativeControlBindingRow, kCreativeInputBindingCapacity> rows{};
  std::size_t count = 0;
  bool capacityExceeded = false;

  [[nodiscard]] std::span<const CreativeControlBindingRow>
  items() const noexcept {
    return {rows.data(), count};
  }
};

struct CreativeControlRebindRequest {
  std::uint16_t group = 0;
  CreativeInputKey trigger = CreativeInputKey::Unbound;
  CreativeInputModifierMask modifiers = kCreativeInputModifierNone;
  CreativeControlConflictPolicy conflictPolicy =
      CreativeControlConflictPolicy::Reject;
};

struct CreativeControlRebindReceipt {
  CreativeControlRebindStatus status =
      CreativeControlRebindStatus::InvalidProfile;
  std::uint16_t group = 0;
  CreativeInputKey previousTrigger = CreativeInputKey::Unbound;
  CreativeInputKey requestedTrigger = CreativeInputKey::Unbound;
  std::array<std::uint16_t, kCreativeInputBindingCapacity> conflictGroups{};
  std::size_t conflictCount = 0;
  bool changed = false;

  [[nodiscard]] std::span<const std::uint16_t>
  conflicts() const noexcept {
    return {conflictGroups.data(), conflictCount};
  }
};

[[nodiscard]] std::string_view toString(CreativeControlDevice device) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeControlConflictPolicy policy) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeControlRebindStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeControlSettingId setting) noexcept;

[[nodiscard]] CreativeControlProfile makeDefaultCreativeControlProfile();
[[nodiscard]] bool isValidCreativeControlProfile(
    const CreativeControlProfile& profile) noexcept;
[[nodiscard]] CreativeControlBindingList buildCreativeControlBindingList(
    const CreativeControlProfile& profile) noexcept;
[[nodiscard]] const CreativeInputBinding* creativeControlGroupBinding(
    const CreativeControlProfile& profile,
    std::uint16_t group) noexcept;
[[nodiscard]] bool creativeControlActionIsReserved(
    CreativeInputActionId action) noexcept;

[[nodiscard]] CreativeControlRebindReceipt rebindCreativeControl(
    CreativeControlProfile& profile,
    const CreativeControlRebindRequest& request) noexcept;
[[nodiscard]] bool applyStoredCreativeControlChord(
    CreativeControlProfile& profile,
    std::uint16_t group,
    CreativeInputKey trigger,
    CreativeInputModifierMask requiredAllModifiers,
    CreativeInputModifierMask requiredAnyModifiers,
    CreativeInputModifierMask allowedModifiers) noexcept;
[[nodiscard]] bool adjustCreativeControlSetting(
    CreativeControlProfile& profile,
    CreativeControlSettingId setting,
    std::int32_t direction) noexcept;

[[nodiscard]] std::string creativeControlKeyDisplayLabel(
    CreativeInputKey key);
[[nodiscard]] std::string creativeControlBindingDisplayLabel(
    const CreativeControlBindingRow& row,
    CreativeInputPlatform platform);

}  // namespace iggy3d::creative
