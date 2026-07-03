#include "config/MovementDimensionProfile.hpp"

#include <array>
#include <cmath>

namespace iggy3d {
namespace {

// The giant liminal dimension: lighter gravity, and dash AND movement limit raised TOGETHER (coherent
// -- 22.0 x 0.18 = 3.96 <= 4.5). Reviewer's coherent first values, A10-tunable; the POINT is the row
// exists + validates. Starts from earth defaults, then overrides the world's feel.
constexpr MovementDimensionProfile makeGiantLowgrav() {
  MovementDimensionProfile profile;
  profile.id = "giant_lowgrav";
  profile.movementDistanceMeters = 4.500F;
  profile.gravityMetersPerSecondSquared = 11.0F;
  profile.jumpImpulseMetersPerSecond = 19.5F;
  profile.wallRunDurationSeconds = 1.1F;
  profile.dashSpeedMetersPerSecond = 22.0F;
  return profile;
}

// The dimension rows (stable order): earth_standard (today's values, dash trimmed) + giant_lowgrav.
inline constexpr std::array<MovementDimensionProfile, 2> kMovementDimensionProfiles{{
    MovementDimensionProfile{},  // earth_standard (id defaults to "earth_standard")
    makeGiantLowgrav(),
}};

}  // namespace

const MovementDimensionProfile* movementDimensionProfileById(std::string_view id) {
  for (const MovementDimensionProfile& profile : kMovementDimensionProfiles) {
    if (profile.id == id) {
      return &profile;
    }
  }
  return nullptr;
}

bool isCoherentMovementProfile(const MovementDimensionProfile& profile,
                               float resolvedMovementDistanceMeters) {
  if (!std::isfinite(profile.dashSpeedMetersPerSecond) ||
      !std::isfinite(profile.dashDurationSeconds) ||
      !std::isfinite(resolvedMovementDistanceMeters)) {
    return false;
  }
  return profile.dashSpeedMetersPerSecond * profile.dashDurationSeconds <=
         resolvedMovementDistanceMeters;
}

}  // namespace iggy3d
