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
  GroundAcceleration,
  GroundDeceleration,
  AirControl,
  JumpImpulse,
  Gravity,
  CoyoteTime,
  JumpBuffer,
  JumpCutMultiplier,
  FallGravityMultiplier,
  LookSensitivity,
  InvertLook,
  DashSpeed,
  DashDuration,
  DashCooldown,
  WallRunMinSpeed,
  WallRunMaxNormalY,
  WallRunDuration,
  WallRunGravityMultiplier,
  WallRunSpeedMultiplier,
};

enum class ProductGameplayMovementTuningFieldKind : std::uint8_t {
  Scalar,
  Toggle,
};

enum class ProductGameplayMovementState : std::uint8_t {
  IdleGrounded,
  MovingGrounded,
  Jumping,
  Rising,
  Falling,
  AirborneControl,
  WallRunning,
  BlockedOrSliding,
};

struct ProductGameplayMovementStateDescriptor {
  ProductGameplayMovementState state = ProductGameplayMovementState::IdleGrounded;
  std::string_view name = "idle_grounded";
  std::string_view hudLabel = "idle grounded";
};

inline constexpr std::array<ProductGameplayMovementStateDescriptor, 8U>
    kProductGameplayMovementStateDescriptors{{
        {ProductGameplayMovementState::IdleGrounded,
         "idle_grounded",
         "idle grounded"},
        {ProductGameplayMovementState::MovingGrounded,
         "moving_grounded",
         "moving grounded"},
        {ProductGameplayMovementState::Jumping, "jumping", "jumping"},
        {ProductGameplayMovementState::Rising, "rising", "rising"},
        {ProductGameplayMovementState::Falling, "falling", "falling"},
        {ProductGameplayMovementState::AirborneControl,
         "airborne_control",
         "airborne control"},
        {ProductGameplayMovementState::WallRunning,
         "wall_running",
         "wall running"},
        {ProductGameplayMovementState::BlockedOrSliding,
         "blocked_or_sliding",
         "blocked or sliding"},
    }};

constexpr std::size_t productGameplayMovementStateDescriptorCount() {
  return kProductGameplayMovementStateDescriptors.size();
}

constexpr std::size_t productGameplayMovementStateIndex(
    ProductGameplayMovementState state) {
  const std::size_t index = static_cast<std::size_t>(state);
  return index < productGameplayMovementStateDescriptorCount() ? index : 0U;  // branch-gate: BG-1161
}

constexpr const ProductGameplayMovementStateDescriptor&
productGameplayMovementStateDescriptor(ProductGameplayMovementState state) {
  return kProductGameplayMovementStateDescriptors[
      productGameplayMovementStateIndex(state)];
}

constexpr std::string_view productGameplayMovementStateName(
    ProductGameplayMovementState state) {
  return productGameplayMovementStateDescriptor(state).name;
}

constexpr std::string_view productGameplayMovementStateHudLabel(
    ProductGameplayMovementState state) {
  return productGameplayMovementStateDescriptor(state).hudLabel;
}

struct ProductWallRunStatusDescriptor {
  std::string_view key = "wall_run_inactive";
  std::string_view hudLabel = "wall run inactive";
};

inline constexpr std::array<ProductWallRunStatusDescriptor, 16U>
    kProductWallRunStatusDescriptors{{
        {"wall_run_not_checked", "wall run not checked"},
        {"wall_run_candidate", "wall run candidate"},
        {"wall_run_active", "wall run active"},
        {"wall_run_started", "wall run started"},
        {"wall_run_expired", "wall run expired"},
        {"wall_run_input_stopped", "wall run input stopped"},
        {"wall_run_input_away", "wall run input away"},
        {"wall_run_inactive", "wall run inactive"},
        {"wall_run_landed", "wall run landed"},
        {"wall_run_exit_jump", "wall run exit jump"},
        {"wall_run_grounded", "wall run grounded"},
        {"wall_run_low_speed", "wall run low speed"},
        {"wall_run_no_surfaces", "wall run no surfaces"},
        {"wall_run_missing_player", "wall run missing player"},
        {"wall_run_no_wall_contact", "wall run no wall contact"},
        {"wall_run_not_along_wall", "wall run not along wall"},
    }};

constexpr const ProductWallRunStatusDescriptor* findProductWallRunStatusDescriptor(
    std::string_view key) {
  for (const ProductWallRunStatusDescriptor& descriptor :
       kProductWallRunStatusDescriptors) {
    // branch-gate: BG-1157
    if (descriptor.key == key) {
      return &descriptor;
    }
  }
  return nullptr;
}

struct ProductGameplayMovementTuning {
  std::string_view walkProfile = "manual_first_person";
  std::string_view sprintProfile = "manual_first_person_sprint";
  std::string_view dashProfile = "manual_first_person_dash";

  // Player feel tuning lives here. Adjust these values when testing movement.
  float walkSpeedMetersPerSecond = 3.3F;
  float sprintSpeedMetersPerSecond = 6.2F;
  float groundAccelerationMetersPerSecondSquared = 400.0F;
  float groundDecelerationMetersPerSecondSquared = 400.0F;
  float airControlMultiplier = 1.0F;
  float inputStepSeconds = 1.0F / 60.0F;

  float jumpImpulseMetersPerSecond = 15.8F;
  float gravityMetersPerSecondSquared = 18.0F;
  float coyoteTimeSeconds = 0.10F;
  float jumpBufferSeconds = 0.10F;
  float jumpCutMultiplier = 0.50F;
  float fallGravityMultiplier = 1.60F;

  float lookSensitivity = 1.0F;
  float invertLookEnabled = 0.0F;

  float dashSpeedMetersPerSecond = 18.5F;
  float dashDurationSeconds = 0.18F;
  float dashCooldownSeconds = 0.45F;

  float wallRunMinSpeedMetersPerSecond = 2.0F;
  float wallRunMaxWallNormalY = 0.25F;
  float wallRunDurationSeconds = 0.75F;
  float wallRunGravityMultiplier = 0.25F;
  float wallRunSpeedMultiplier = 1.0F;

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
  ProductGameplayMovementTuningFieldKind kind =
      ProductGameplayMovementTuningFieldKind::Scalar;
  float minValue = 0.1F;
  float maxValue = 12.0F;
  float step = 0.1F;
};

inline constexpr std::array<ProductGameplayMovementTuningFieldDescriptor, 21U>
    kProductGameplayMovementTuningFields{{
        {ProductGameplayMovementTuningField::WalkSpeed,
         "walk_speed_mps",
         "WALK",
         &ProductGameplayMovementTuning::walkSpeedMetersPerSecond,
         ProductGameplayMovementTuningFieldKind::Scalar,
         0.1F,
         12.0F,
         0.1F},
        {ProductGameplayMovementTuningField::SprintSpeed,
         "sprint_speed_mps",
         "SPRINT",
         &ProductGameplayMovementTuning::sprintSpeedMetersPerSecond,
         ProductGameplayMovementTuningFieldKind::Scalar,
         0.1F,
         18.0F,
         0.1F},
        {ProductGameplayMovementTuningField::GroundAcceleration,
         "ground_acceleration_mps2",
         "ACCEL",
         &ProductGameplayMovementTuning::groundAccelerationMetersPerSecondSquared,
         ProductGameplayMovementTuningFieldKind::Scalar,
         1.0F,
         400.0F,
         5.0F},
        {ProductGameplayMovementTuningField::GroundDeceleration,
         "ground_deceleration_mps2",
         "STOP",
         &ProductGameplayMovementTuning::groundDecelerationMetersPerSecondSquared,
         ProductGameplayMovementTuningFieldKind::Scalar,
         1.0F,
         400.0F,
         5.0F},
        {ProductGameplayMovementTuningField::AirControl,
         "air_control",
         "AIR CTRL",
         &ProductGameplayMovementTuning::airControlMultiplier,
         ProductGameplayMovementTuningFieldKind::Scalar,
         0.0F,
         2.0F,
         0.05F},
        {ProductGameplayMovementTuningField::JumpImpulse,
         "jump_impulse_mps",
         "JUMP",
         &ProductGameplayMovementTuning::jumpImpulseMetersPerSecond,
         ProductGameplayMovementTuningFieldKind::Scalar,
         0.1F,
         24.0F,
         0.1F},
        {ProductGameplayMovementTuningField::Gravity,
         "gravity_mps2",
         "GRAV",
         &ProductGameplayMovementTuning::gravityMetersPerSecondSquared,
         ProductGameplayMovementTuningFieldKind::Scalar,
         1.0F,
         40.0F,
         0.5F},
        {ProductGameplayMovementTuningField::CoyoteTime,
         "coyote_time_s",
         "COYOTE",
         &ProductGameplayMovementTuning::coyoteTimeSeconds,
         ProductGameplayMovementTuningFieldKind::Scalar,
         0.0F,
         0.25F,
         0.01F},
        {ProductGameplayMovementTuningField::JumpBuffer,
         "jump_buffer_s",
         "BUFFER",
         &ProductGameplayMovementTuning::jumpBufferSeconds,
         ProductGameplayMovementTuningFieldKind::Scalar,
         0.0F,
         0.25F,
         0.01F},
        {ProductGameplayMovementTuningField::JumpCutMultiplier,
         "jump_cut_multiplier",
         "CUT",
         &ProductGameplayMovementTuning::jumpCutMultiplier,
         ProductGameplayMovementTuningFieldKind::Scalar,
         0.1F,
         1.0F,
         0.05F},
        {ProductGameplayMovementTuningField::FallGravityMultiplier,
         "fall_gravity_multiplier",
         "FALL",
         &ProductGameplayMovementTuning::fallGravityMultiplier,
         ProductGameplayMovementTuningFieldKind::Scalar,
         1.0F,
         4.0F,
         0.1F},
        {ProductGameplayMovementTuningField::LookSensitivity,
         "look_sensitivity",
         "LOOK",
         &ProductGameplayMovementTuning::lookSensitivity,
         ProductGameplayMovementTuningFieldKind::Scalar,
         0.1F,
         4.0F,
         0.1F},
        {ProductGameplayMovementTuningField::InvertLook,
         "invert_look",
         "INVERT Y",
         &ProductGameplayMovementTuning::invertLookEnabled,
         ProductGameplayMovementTuningFieldKind::Toggle,
         0.0F,
         1.0F,
         1.0F},
        {ProductGameplayMovementTuningField::DashSpeed,
         "dash_speed_mps",
         "DASH",
         &ProductGameplayMovementTuning::dashSpeedMetersPerSecond,
         ProductGameplayMovementTuningFieldKind::Scalar,
         0.1F,
         40.0F,
         0.5F},
        {ProductGameplayMovementTuningField::DashDuration,
         "dash_duration_s",
         "DASHDUR",
         &ProductGameplayMovementTuning::dashDurationSeconds,
         ProductGameplayMovementTuningFieldKind::Scalar,
         0.02F,
         1.0F,
         0.02F},
        {ProductGameplayMovementTuningField::DashCooldown,
         "dash_cooldown_s",
         "DASHCD",
         &ProductGameplayMovementTuning::dashCooldownSeconds,
         ProductGameplayMovementTuningFieldKind::Scalar,
         0.0F,
         3.0F,
         0.05F},
        {ProductGameplayMovementTuningField::WallRunMinSpeed,
         "wall_run_min_speed_mps",
         "WALLRUN SPD",
         &ProductGameplayMovementTuning::wallRunMinSpeedMetersPerSecond,
         ProductGameplayMovementTuningFieldKind::Scalar,
         0.1F,
         12.0F,
         0.1F},
        {ProductGameplayMovementTuningField::WallRunMaxNormalY,
         "wall_run_max_normal_y",
         "WALLRUN NY",
         &ProductGameplayMovementTuning::wallRunMaxWallNormalY,
         ProductGameplayMovementTuningFieldKind::Scalar,
         0.0F,
         1.0F,
         0.05F},
        {ProductGameplayMovementTuningField::WallRunDuration,
         "wall_run_duration_s",
         "WALLRUN DUR",
         &ProductGameplayMovementTuning::wallRunDurationSeconds,
         ProductGameplayMovementTuningFieldKind::Scalar,
         0.1F,
         2.0F,
         0.05F},
        {ProductGameplayMovementTuningField::WallRunGravityMultiplier,
         "wall_run_gravity_multiplier",
         "WALLRUN GRAV",
         &ProductGameplayMovementTuning::wallRunGravityMultiplier,
         ProductGameplayMovementTuningFieldKind::Scalar,
         0.0F,
         1.0F,
         0.05F},
        {ProductGameplayMovementTuningField::WallRunSpeedMultiplier,
         "wall_run_speed_multiplier",
         "WALLRUN SPD*",
         &ProductGameplayMovementTuning::wallRunSpeedMultiplier,
         ProductGameplayMovementTuningFieldKind::Scalar,
         0.25F,
         2.0F,
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

inline bool productGameplayMovementTuningInvertLook(
    const ProductGameplayMovementTuning& tuning) {
  return tuning.invertLookEnabled >= 0.5F;
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
  // branch-gate: BG-1208
  if (descriptor.kind == ProductGameplayMovementTuningFieldKind::Toggle) {
    value = value >= 0.5F ? 0.0F : 1.0F;  // branch-gate: BG-1208
    return value;
  }
  value = std::clamp(value + descriptor.step * static_cast<float>(direction),
                     descriptor.minValue,
                     descriptor.maxValue);
  return value;
}

}  // namespace iggy3d
