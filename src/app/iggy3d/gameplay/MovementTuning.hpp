#pragma once

#include <string_view>

namespace iggy3d {

struct ProductGameplayMovementTuning {
  std::string_view walkProfile = "manual_first_person";
  std::string_view sprintProfile = "manual_first_person_sprint";
  std::string_view dashProfile = "manual_first_person_dash";

  // Player feel tuning lives here. Adjust these values when testing movement.
  float walkSpeedMetersPerSecond = 3.3F;
  float sprintSpeedMetersPerSecond = 3.2F;
  float inputStepSeconds = 1.0F / 60.0F;

  float jumpImpulseMetersPerSecond = 5.8F;
  float gravityMetersPerSecondSquared = 18.0F;

  float dashSpeedMetersPerSecond = 9.5F;
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

}  // namespace iggy3d
