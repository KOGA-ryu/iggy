#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace iggy3d {

enum class ProductGameplayMovementTuningField : std::uint8_t {
  WalkSpeed,
  SprintSpeed,
  JumpImpulse,
  Gravity,
  DashSpeed,
  DashDuration,
  DashCooldown,
};

struct ProductGameplayMovementTuning {
  std::string_view walkProfile = "manual_first_person";
  std::string_view sprintProfile = "manual_first_person_sprint";
  std::string_view dashProfile = "manual_first_person_dash";

  // Player feel tuning lives here. Adjust these values when testing movement.
  float walkSpeedMetersPerSecond = 3.3F;
  float sprintSpeedMetersPerSecond = 6.2F;
  float inputStepSeconds = 1.0F / 60.0F;

  float jumpImpulseMetersPerSecond = 15.8F;
  float gravityMetersPerSecondSquared = 18.0F;

  float dashSpeedMetersPerSecond = 18.5F;
  float dashDurationSeconds = 0.18F;
  float dashCooldownSeconds = 0.45F;

  float wallJumpProbeMeters = 0.58F;
  float wallJumpPushMeters = 1.20F;
  float wallJumpRiseMeters = 0.45F;
  float wallJumpMinAirborneHeightMeters = 0.20F;
};

inline constexpr ProductGameplayMovementTuning kProductGameplayMovementTuning{};

constexpr const ProductGameplayMovementTuning& productGameplayMovementTuning() {
  return kProductGameplayMovementTuning;
}

struct ProductGameplayMovementTuningFieldDescriptor {
  ProductGameplayMovementTuningField field = ProductGameplayMovementTuningField::WalkSpeed;
  std::string_view name = "walk_speed_mps";
  std::string_view label = "WALK";
  float ProductGameplayMovementTuning::* value =
      &ProductGameplayMovementTuning::walkSpeedMetersPerSecond;
  float minValue = 0.1F;
  float maxValue = 12.0F;
  float step = 0.1F;
};

inline constexpr std::array<ProductGameplayMovementTuningFieldDescriptor, 7U>
    kProductGameplayMovementTuningFields{{
        {ProductGameplayMovementTuningField::WalkSpeed,
         "walk_speed_mps",
         "WALK",
         &ProductGameplayMovementTuning::walkSpeedMetersPerSecond,
         0.1F,
         12.0F,
         0.1F},
        {ProductGameplayMovementTuningField::SprintSpeed,
         "sprint_speed_mps",
         "SPRINT",
         &ProductGameplayMovementTuning::sprintSpeedMetersPerSecond,
         0.1F,
         18.0F,
         0.1F},
        {ProductGameplayMovementTuningField::JumpImpulse,
         "jump_impulse_mps",
         "JUMP",
         &ProductGameplayMovementTuning::jumpImpulseMetersPerSecond,
         0.1F,
         24.0F,
         0.1F},
        {ProductGameplayMovementTuningField::Gravity,
         "gravity_mps2",
         "GRAV",
         &ProductGameplayMovementTuning::gravityMetersPerSecondSquared,
         1.0F,
         40.0F,
         0.5F},
        {ProductGameplayMovementTuningField::DashSpeed,
         "dash_speed_mps",
         "DASH",
         &ProductGameplayMovementTuning::dashSpeedMetersPerSecond,
         0.1F,
         40.0F,
         0.5F},
        {ProductGameplayMovementTuningField::DashDuration,
         "dash_duration_s",
         "DASHDUR",
         &ProductGameplayMovementTuning::dashDurationSeconds,
         0.02F,
         1.0F,
         0.02F},
        {ProductGameplayMovementTuningField::DashCooldown,
         "dash_cooldown_s",
         "DASHCD",
         &ProductGameplayMovementTuning::dashCooldownSeconds,
         0.0F,
         3.0F,
         0.05F},
    }};

constexpr std::size_t productGameplayMovementTuningFieldCount() {
  return kProductGameplayMovementTuningFields.size();
}

constexpr std::size_t productGameplayMovementTuningFieldIndex(
    ProductGameplayMovementTuningField field) {
  const std::size_t index = static_cast<std::size_t>(field);
  // branch-gate: BG-1208
  return index < productGameplayMovementTuningFieldCount() ? index : 0U;
}

constexpr const ProductGameplayMovementTuningFieldDescriptor&
productGameplayMovementTuningFieldDescriptor(
    ProductGameplayMovementTuningField field) {
  return kProductGameplayMovementTuningFields[
      productGameplayMovementTuningFieldIndex(field)];
}

constexpr std::string_view productGameplayMovementTuningFieldName(
    ProductGameplayMovementTuningField field) {
  return productGameplayMovementTuningFieldDescriptor(field).name;
}

constexpr std::string_view productGameplayMovementTuningFieldLabel(
    ProductGameplayMovementTuningField field) {
  return productGameplayMovementTuningFieldDescriptor(field).label;
}

inline float productGameplayMovementTuningFieldValue(
    const ProductGameplayMovementTuning& tuning,
    ProductGameplayMovementTuningField field) {
  const auto& descriptor = productGameplayMovementTuningFieldDescriptor(field);
  return tuning.*(descriptor.value);
}

constexpr ProductGameplayMovementTuningField nextProductGameplayMovementTuningField(
    ProductGameplayMovementTuningField field) {
  const std::size_t next =
      (productGameplayMovementTuningFieldIndex(field) + 1U) %
      productGameplayMovementTuningFieldCount();
  return kProductGameplayMovementTuningFields[next].field;
}

constexpr ProductGameplayMovementTuningField previousProductGameplayMovementTuningField(
    ProductGameplayMovementTuningField field) {
  const std::size_t index = productGameplayMovementTuningFieldIndex(field);
  // branch-gate: BG-1208
  const std::size_t previous =
      index == 0U ? productGameplayMovementTuningFieldCount() - 1U : index - 1U;
  return kProductGameplayMovementTuningFields[previous].field;
}

inline float adjustProductGameplayMovementTuning(
    ProductGameplayMovementTuning& tuning,
    ProductGameplayMovementTuningField field,
    int direction) {
  const auto& descriptor = productGameplayMovementTuningFieldDescriptor(field);
  float& value = tuning.*(descriptor.value);
  value = std::clamp(value + descriptor.step * static_cast<float>(direction),
                     descriptor.minValue,
                     descriptor.maxValue);
  return value;
}

}  // namespace iggy3d
