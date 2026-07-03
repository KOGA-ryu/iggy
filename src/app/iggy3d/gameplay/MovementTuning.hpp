#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "config/MovementDimensionProfile.hpp"

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
  // M-LAB s1: the wall-jump quad -- appended at the END so the prior ordinals (== array indices) are
  // stable. These complete full feel coverage (every float field but inputStepSeconds is now tunable).
  WallJumpProbe,
  WallJumpPush,
  WallJumpRise,
  WallJumpMinAirborneHeight,
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

  float dashSpeedMetersPerSecond = 16.6F;  // MA1 dash fix: 16.6 x 0.18 = 2.988 m <= movementDistance 3.0
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

// M-LAB s1 anti-drift guard: the descriptor-coverage test counts float fields by sizeof arithmetic
// (sizeof(struct) - 3*sizeof(string_view)) / sizeof(float), which is only valid if the layout is
// padding-free. 48 B ids + 104 B floats = 152 B, 8-aligned -- enforce it at compile time so a future
// field that changes the layout can't silently break the count.
static_assert(sizeof(ProductGameplayMovementTuning) ==
                  3U * sizeof(std::string_view) + 26U * sizeof(float),
              "ProductGameplayMovementTuning layout drifted -- update the descriptor coverage math");

inline constexpr ProductGameplayMovementTuning kProductGameplayMovementTuning{};

// M-LAB s1: fields DELIBERATELY excluded from the tuning cockpit. inputStepSeconds is the fixed tick
// step -- constitution, not feel -- so it gets no descriptor; the coverage test counts it here so the
// (descriptors + exclusions == float fields) invariant still holds.
inline constexpr std::array<float ProductGameplayMovementTuning::*, 1U>
    kExcludedMovementTuningFields{{
        &ProductGameplayMovementTuning::inputStepSeconds,
    }};

constexpr const ProductGameplayMovementTuning& productGameplayMovementTuning() {
  return kProductGameplayMovementTuning;
}

// MA1: map the runtime/config-side MovementDimensionProfile MIRROR onto the app tuning at window
// init (the injection point). Copies the 26 numerics + 3 ids (NOT movementDistanceMeters -- that is
// runtime config, not app feel). earth_standard maps BYTE-IDENTICAL to kProductGameplayMovementTuning
// (a test guards this anti-drift). This is the ONLY place the two structs meet.
inline void applyMovementDimensionProfileToTuning(const MovementDimensionProfile& profile,
                                                  ProductGameplayMovementTuning& tuning) {
  tuning.walkProfile = profile.walkProfile;
  tuning.sprintProfile = profile.sprintProfile;
  tuning.dashProfile = profile.dashProfile;
  tuning.walkSpeedMetersPerSecond = profile.walkSpeedMetersPerSecond;
  tuning.sprintSpeedMetersPerSecond = profile.sprintSpeedMetersPerSecond;
  tuning.groundAccelerationMetersPerSecondSquared = profile.groundAccelerationMetersPerSecondSquared;
  tuning.groundDecelerationMetersPerSecondSquared = profile.groundDecelerationMetersPerSecondSquared;
  tuning.airControlMultiplier = profile.airControlMultiplier;
  tuning.inputStepSeconds = profile.inputStepSeconds;
  tuning.jumpImpulseMetersPerSecond = profile.jumpImpulseMetersPerSecond;
  tuning.gravityMetersPerSecondSquared = profile.gravityMetersPerSecondSquared;
  tuning.coyoteTimeSeconds = profile.coyoteTimeSeconds;
  tuning.jumpBufferSeconds = profile.jumpBufferSeconds;
  tuning.jumpCutMultiplier = profile.jumpCutMultiplier;
  tuning.fallGravityMultiplier = profile.fallGravityMultiplier;
  tuning.lookSensitivity = profile.lookSensitivity;
  tuning.invertLookEnabled = profile.invertLookEnabled;
  tuning.dashSpeedMetersPerSecond = profile.dashSpeedMetersPerSecond;
  tuning.dashDurationSeconds = profile.dashDurationSeconds;
  tuning.dashCooldownSeconds = profile.dashCooldownSeconds;
  tuning.wallRunMinSpeedMetersPerSecond = profile.wallRunMinSpeedMetersPerSecond;
  tuning.wallRunMaxWallNormalY = profile.wallRunMaxWallNormalY;
  tuning.wallRunDurationSeconds = profile.wallRunDurationSeconds;
  tuning.wallRunGravityMultiplier = profile.wallRunGravityMultiplier;
  tuning.wallRunSpeedMultiplier = profile.wallRunSpeedMultiplier;
  tuning.wallJumpProbeMeters = profile.wallJumpProbeMeters;
  tuning.wallJumpPushMeters = profile.wallJumpPushMeters;
  tuning.wallJumpRiseMeters = profile.wallJumpRiseMeters;
  tuning.wallJumpMinAirborneHeightMeters = profile.wallJumpMinAirborneHeightMeters;
}

// M-LAB s1 export assembly (app-side -- may see both structs): the INVERSE of applyMovementDimension
// ProfileToTuning. The 26 numerics + 3 ids come from the LIVE window `tuning` (what the designer just
// felt); movementDistanceMeters + the sneak multipliers are runtime-only fields the cockpit never
// tunes, so they carry from `lastAppliedRow` (the row last hot-swapped in); `id = exportId`. NOTE:
// the result's string_view fields (id + the 3 profile names) reference `exportId`/`tuning` -- keep
// them alive while the result is used (the caller serializes it immediately).
inline MovementDimensionProfile movementDimensionProfileFromCockpit(
    const ProductGameplayMovementTuning& tuning,
    const MovementDimensionProfile& lastAppliedRow,
    std::string_view exportId) {
  MovementDimensionProfile row;
  row.id = exportId;
  row.movementDistanceMeters = lastAppliedRow.movementDistanceMeters;
  row.walkSpeedMetersPerSecond = tuning.walkSpeedMetersPerSecond;
  row.sprintSpeedMetersPerSecond = tuning.sprintSpeedMetersPerSecond;
  row.groundAccelerationMetersPerSecondSquared = tuning.groundAccelerationMetersPerSecondSquared;
  row.groundDecelerationMetersPerSecondSquared = tuning.groundDecelerationMetersPerSecondSquared;
  row.airControlMultiplier = tuning.airControlMultiplier;
  row.inputStepSeconds = tuning.inputStepSeconds;
  row.jumpImpulseMetersPerSecond = tuning.jumpImpulseMetersPerSecond;
  row.gravityMetersPerSecondSquared = tuning.gravityMetersPerSecondSquared;
  row.coyoteTimeSeconds = tuning.coyoteTimeSeconds;
  row.jumpBufferSeconds = tuning.jumpBufferSeconds;
  row.jumpCutMultiplier = tuning.jumpCutMultiplier;
  row.fallGravityMultiplier = tuning.fallGravityMultiplier;
  row.lookSensitivity = tuning.lookSensitivity;
  row.invertLookEnabled = tuning.invertLookEnabled;
  row.dashSpeedMetersPerSecond = tuning.dashSpeedMetersPerSecond;
  row.dashDurationSeconds = tuning.dashDurationSeconds;
  row.dashCooldownSeconds = tuning.dashCooldownSeconds;
  row.wallRunMinSpeedMetersPerSecond = tuning.wallRunMinSpeedMetersPerSecond;
  row.wallRunMaxWallNormalY = tuning.wallRunMaxWallNormalY;
  row.wallRunDurationSeconds = tuning.wallRunDurationSeconds;
  row.wallRunGravityMultiplier = tuning.wallRunGravityMultiplier;
  row.wallRunSpeedMultiplier = tuning.wallRunSpeedMultiplier;
  row.wallJumpProbeMeters = tuning.wallJumpProbeMeters;
  row.wallJumpPushMeters = tuning.wallJumpPushMeters;
  row.wallJumpRiseMeters = tuning.wallJumpRiseMeters;
  row.wallJumpMinAirborneHeightMeters = tuning.wallJumpMinAirborneHeightMeters;
  row.walkProfile = tuning.walkProfile;
  row.sprintProfile = tuning.sprintProfile;
  row.dashProfile = tuning.dashProfile;
  row.sneakSpeedMultiplier = lastAppliedRow.sneakSpeedMultiplier;
  row.sneakLoudnessMultiplier = lastAppliedRow.sneakLoudnessMultiplier;
  return row;
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

inline constexpr std::array<ProductGameplayMovementTuningFieldDescriptor, 25U>
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
        // M-LAB s1: the wall-jump quad completes feel coverage (coherent ranges, appended in enum order).
        {ProductGameplayMovementTuningField::WallJumpProbe,
         "wall_jump_probe_m",
         "WALLJMP PROBE",
         &ProductGameplayMovementTuning::wallJumpProbeMeters,
         ProductGameplayMovementTuningFieldKind::Scalar,
         0.1F,
         2.0F,
         0.02F},
        {ProductGameplayMovementTuningField::WallJumpPush,
         "wall_jump_push_m",
         "WALLJMP PUSH",
         &ProductGameplayMovementTuning::wallJumpPushMeters,
         ProductGameplayMovementTuningFieldKind::Scalar,
         0.1F,
         3.0F,
         0.05F},
        {ProductGameplayMovementTuningField::WallJumpRise,
         "wall_jump_rise_m",
         "WALLJMP RISE",
         &ProductGameplayMovementTuning::wallJumpRiseMeters,
         ProductGameplayMovementTuningFieldKind::Scalar,
         0.0F,
         2.0F,
         0.05F},
        {ProductGameplayMovementTuningField::WallJumpMinAirborneHeight,
         "wall_jump_min_airborne_height_m",
         "WALLJMP MINAIR",
         &ProductGameplayMovementTuning::wallJumpMinAirborneHeightMeters,
         ProductGameplayMovementTuningFieldKind::Scalar,
         0.0F,
         1.0F,
         0.02F},
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
