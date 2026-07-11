#include "app/iggy3d/creative/input/ControllerInput.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <type_traits>

namespace iggy3d::creative {
namespace {

struct StickAxes {
  CreativeControllerAxis x;
  CreativeControllerAxis y;
};

constexpr std::array kStickAxes{
    StickAxes{CreativeControllerAxis::LeftStickX,
              CreativeControllerAxis::LeftStickY},
    StickAxes{CreativeControllerAxis::RightStickX,
              CreativeControllerAxis::RightStickY},
};

[[nodiscard]] constexpr std::size_t axisIndex(
    CreativeControllerAxis axis) noexcept {
  return static_cast<std::size_t>(axis);
}

[[nodiscard]] constexpr std::size_t buttonIndex(
    CreativeControllerButton button) noexcept {
  return static_cast<std::size_t>(button);
}

[[nodiscard]] float finiteClamped(float value,
                                  float minimum,
                                  float maximum) noexcept {
  return std::isfinite(value) ? std::clamp(value, minimum, maximum) : 0.0F;
}

[[nodiscard]] bool validStickProfile(
    const CreativeStickProfile& profile) noexcept {
  return std::isfinite(profile.deadzone) && profile.deadzone >= 0.0F &&
         profile.deadzone < 1.0F &&
         std::isfinite(profile.responseExponent) &&
         profile.responseExponent > 0.0F;
}

[[nodiscard]] float normalizedSignedAxis(std::int16_t value) noexcept {
  return value < 0 ? static_cast<float>(value) / 32768.0F
                   : static_cast<float>(value) / 32767.0F;
}

}  // namespace

static_assert(std::is_trivially_copyable_v<CreativeStickProfile>);
static_assert(std::is_trivially_copyable_v<CreativeStickSignal>);
static_assert(std::is_trivially_copyable_v<CreativeControllerLookDelta>);

void setCreativeControllerAxis(CreativeControllerSample& sample,
                               CreativeControllerAxis axis,
                               float value) noexcept {
  if (axis != CreativeControllerAxis::Count) {
    sample.axes[axisIndex(axis)] = value;
  }
}

void setCreativeControllerButton(CreativeControllerSample& sample,
                                 CreativeControllerButton button,
                                 bool down) noexcept {
  if (button != CreativeControllerButton::Count) {
    sample.buttonsDown[buttonIndex(button)] = down;
  }
}

float creativeControllerAxis(const CreativeControllerFrame& frame,
                             CreativeControllerAxis axis) noexcept {
  return axis == CreativeControllerAxis::Count ? 0.0F
                                                : frame.axes[axisIndex(axis)];
}

bool creativeControllerButtonDown(const CreativeControllerFrame& frame,
                                  CreativeControllerButton button) noexcept {
  return button != CreativeControllerButton::Count &&
         frame.next.buttonsDown[buttonIndex(button)];
}

bool creativeControllerButtonPressed(const CreativeControllerFrame& frame,
                                     CreativeControllerButton button) noexcept {
  return button != CreativeControllerButton::Count &&
         frame.pressed[buttonIndex(button)];
}

bool creativeControllerButtonReleased(
    const CreativeControllerFrame& frame,
    CreativeControllerButton button) noexcept {
  return button != CreativeControllerButton::Count &&
         frame.released[buttonIndex(button)];
}

float canonicalCreativeStickComponent(
    std::int16_t rawValue,
    CreativeStickComponent component) noexcept {
  if (component == CreativeStickComponent::Count) {
    return 0.0F;
  }
  const float normalized = normalizedSignedAxis(rawValue);
  return component == CreativeStickComponent::Y ? -normalized : normalized;
}

CreativeStickSignal shapeCreativeControllerStick(
    float canonicalX,
    float canonicalY,
    CreativeStickProfile profile) noexcept {
  if (!std::isfinite(canonicalX) || !std::isfinite(canonicalY) ||
      !validStickProfile(profile)) {
    return {};
  }

  float x = std::clamp(canonicalX, -1.0F, 1.0F);
  float y = std::clamp(canonicalY, -1.0F, 1.0F);
  float magnitude = std::hypot(x, y);
  if (magnitude > 1.0F) {
    x /= magnitude;
    y /= magnitude;
    magnitude = 1.0F;
  }
  if (magnitude <= profile.deadzone) {
    return {};
  }

  const float remappedMagnitude =
      (magnitude - profile.deadzone) / (1.0F - profile.deadzone);
  const float shapedMagnitude =
      std::pow(std::clamp(remappedMagnitude, 0.0F, 1.0F),
               profile.responseExponent);
  const float directionScale = shapedMagnitude / magnitude;
  CreativeStickSignal signal;
  signal.x = x * directionScale * (profile.invertX ? -1.0F : 1.0F);
  signal.y = y * directionScale * (profile.invertY ? -1.0F : 1.0F);
  signal.magnitude = shapedMagnitude;
  signal.active = shapedMagnitude > 0.0F;
  return signal;
}

CreativeStickSignal creativeControllerStick(
    const CreativeControllerFrame& frame,
    CreativeControllerStick stick,
    CreativeStickProfile profile) noexcept {
  if (!frame.next.connected || stick == CreativeControllerStick::Count) {
    return {};
  }
  const StickAxes axes = kStickAxes[static_cast<std::size_t>(stick)];
  return shapeCreativeControllerStick(creativeControllerAxis(frame, axes.x),
                                      creativeControllerAxis(frame, axes.y),
                                      profile);
}

CreativeControllerLookDelta creativeControllerLookDelta(
    CreativeStickSignal signal,
    float degreesPerUnit) noexcept {
  if (!signal.active || !std::isfinite(signal.x) ||
      !std::isfinite(signal.y) || !std::isfinite(degreesPerUnit) ||
      degreesPerUnit < 0.0F) {
    return {};
  }
  return {signal.x * degreesPerUnit, signal.y * degreesPerUnit};
}

CreativeControllerFrame stepCreativeControllerInput(
    CreativeControllerState previous,
    const CreativeControllerSample& current) noexcept {
  CreativeControllerFrame frame;
  frame.next.connected = current.connected;
  if (current.connected) {
    constexpr std::array stickAxes{
        CreativeControllerAxis::LeftStickX,
        CreativeControllerAxis::LeftStickY,
        CreativeControllerAxis::RightStickX,
        CreativeControllerAxis::RightStickY,
    };
    for (CreativeControllerAxis axis : stickAxes) {
      const std::size_t index = axisIndex(axis);
      frame.axes[index] = finiteClamped(current.axes[index], -1.0F, 1.0F);
    }

    constexpr std::array triggerAxes{
        CreativeControllerAxis::LeftTrigger,
        CreativeControllerAxis::RightTrigger,
    };
    for (CreativeControllerAxis axis : triggerAxes) {
      const std::size_t index = axisIndex(axis);
      frame.axes[index] = finiteClamped(current.axes[index], 0.0F, 1.0F);
    }
    frame.next.buttonsDown = current.buttonsDown;
    frame.next.buttonsDown[buttonIndex(CreativeControllerButton::LeftTrigger)] =
        creativeControllerAxis(frame, CreativeControllerAxis::LeftTrigger) >=
        kCreativeControllerTriggerThreshold;
    frame.next
        .buttonsDown[buttonIndex(CreativeControllerButton::RightTrigger)] =
        creativeControllerAxis(frame, CreativeControllerAxis::RightTrigger) >=
        kCreativeControllerTriggerThreshold;
  }

  for (std::size_t index = 0; index < kCreativeControllerButtonCount; ++index) {
    frame.pressed[index] =
        frame.next.buttonsDown[index] && !previous.buttonsDown[index];
    frame.released[index] =
        !frame.next.buttonsDown[index] && previous.buttonsDown[index];
  }
  return frame;
}

}  // namespace iggy3d::creative
