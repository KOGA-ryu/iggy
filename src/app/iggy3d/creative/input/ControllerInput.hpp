#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace iggy3d::creative {

// Stick components use one controller-independent convention: X is positive
// right and Y is positive up. Hardware adapters convert to this convention.
enum class CreativeControllerAxis : std::uint8_t {
  LeftStickX,
  LeftStickY,
  RightStickX,
  RightStickY,
  LeftTrigger,
  RightTrigger,
  Count,
};

enum class CreativeControllerStick : std::uint8_t {
  Left,
  Right,
  Count,
};

enum class CreativeStickComponent : std::uint8_t {
  X,
  Y,
  Count,
};

enum class CreativeControllerButton : std::uint8_t {
  South,
  East,
  West,
  North,
  Back,
  Start,
  LeftStick,
  RightStick,
  LeftShoulder,
  RightShoulder,
  DpadUp,
  DpadDown,
  DpadLeft,
  DpadRight,
  LeftTrigger,
  RightTrigger,
  Touchpad,
  Count,
};

inline constexpr std::size_t kCreativeControllerAxisCount =
    static_cast<std::size_t>(CreativeControllerAxis::Count);
inline constexpr std::size_t kCreativeControllerButtonCount =
    static_cast<std::size_t>(CreativeControllerButton::Count);
inline constexpr float kCreativeControllerStickDeadzone = 0.18F;
inline constexpr float kCreativeControllerTriggerThreshold = 0.55F;

struct CreativeStickProfile {
  float deadzone = kCreativeControllerStickDeadzone;
  float responseExponent = 1.0F;
  bool invertX = false;
  bool invertY = false;
};

struct CreativeStickSignal {
  float x = 0.0F;
  float y = 0.0F;
  float magnitude = 0.0F;
  bool active = false;
};

struct CreativeControllerLookDelta {
  float yawDegrees = 0.0F;
  float pitchDegrees = 0.0F;
};

// Hardware adapters populate this platform-neutral snapshot. Trigger button
// state is derived from the corresponding axes by stepCreativeControllerInput.
struct CreativeControllerSample {
  bool connected = false;
  std::array<float, kCreativeControllerAxisCount> axes{};
  std::array<bool, kCreativeControllerButtonCount> buttonsDown{};
};

struct CreativeControllerState {
  bool connected = false;
  std::array<bool, kCreativeControllerButtonCount> buttonsDown{};
};

struct CreativeControllerFrame {
  CreativeControllerState next{};
  std::array<float, kCreativeControllerAxisCount> axes{};
  std::array<bool, kCreativeControllerButtonCount> pressed{};
  std::array<bool, kCreativeControllerButtonCount> released{};
};

void setCreativeControllerAxis(CreativeControllerSample& sample,
                               CreativeControllerAxis axis,
                               float value) noexcept;
void setCreativeControllerButton(CreativeControllerSample& sample,
                                 CreativeControllerButton button,
                                 bool down) noexcept;
[[nodiscard]] float creativeControllerAxis(
    const CreativeControllerFrame& frame,
    CreativeControllerAxis axis) noexcept;
[[nodiscard]] bool creativeControllerButtonDown(
    const CreativeControllerFrame& frame,
    CreativeControllerButton button) noexcept;
[[nodiscard]] bool creativeControllerButtonPressed(
    const CreativeControllerFrame& frame,
    CreativeControllerButton button) noexcept;
[[nodiscard]] bool creativeControllerButtonReleased(
    const CreativeControllerFrame& frame,
    CreativeControllerButton button) noexcept;

// Applies one radial deadzone to the 2D vector, rescales the remaining travel,
// then applies the named consumer profile. Invalid inputs resolve to inactive.
[[nodiscard]] float canonicalCreativeStickComponent(
    std::int16_t rawValue,
    CreativeStickComponent component) noexcept;
[[nodiscard]] CreativeStickSignal shapeCreativeControllerStick(
    float canonicalX,
    float canonicalY,
    CreativeStickProfile profile = {}) noexcept;
[[nodiscard]] CreativeStickSignal creativeControllerStick(
    const CreativeControllerFrame& frame,
    CreativeControllerStick stick,
    CreativeStickProfile profile = {}) noexcept;
[[nodiscard]] CreativeControllerLookDelta creativeControllerLookDelta(
    CreativeStickSignal signal,
    float degreesPerUnit) noexcept;

[[nodiscard]] CreativeControllerFrame stepCreativeControllerInput(
    CreativeControllerState previous,
    const CreativeControllerSample& current) noexcept;

}  // namespace iggy3d::creative
