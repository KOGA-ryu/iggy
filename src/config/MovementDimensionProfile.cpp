#include "config/MovementDimensionProfile.hpp"

#include <array>

namespace iggy3d {
namespace {

// The dimension rows (stable order). GATE 1 ships earth_standard (today's values EXACTLY). GATE 2
// adds giant_lowgrav (the giant liminal world: lighter gravity, everything tuned up together).
inline constexpr std::array<MovementDimensionProfile, 1> kMovementDimensionProfiles{{
    MovementDimensionProfile{},  // earth_standard (id defaults to "earth_standard")
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

}  // namespace iggy3d
