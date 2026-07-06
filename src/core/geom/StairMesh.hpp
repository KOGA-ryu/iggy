#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "core/math/Aabb3.hpp"

namespace iggy3d {

// A parametric staircase spec in LOCAL space (min corner at the origin). Y is up (core convention),
// the run advances along +Z, width spans X. The caller places/rotates the result via a transform --
// this kernel never bakes in a world orientation.
struct StairSpec {
  float width = 0.0F;         // X extent of every step
  float totalRise = 0.0F;     // total height climbed (Y)
  float totalRun = 0.0F;      // total horizontal depth (Z)
  std::uint32_t stepCount = 0;  // number of steps, 1 .. kMaxStairSteps
};

// The generated staircase: one SOLID box per step, each rising from the floor (y=0) to its tread
// height over a distinct, contiguous run slice -- together a watertight solid the bake/collision can
// consume directly. Step i occupies z in [i*stepRun, (i+1)*stepRun], y in [0, (i+1)*stepRise].
struct StairMesh {
  bool ok = false;
  std::string_view reasonCode = "stair_not_generated";
  std::uint32_t stepCount = 0;
  float stepRise = 0.0F;
  float stepRun = 0.0F;
  std::vector<Aabb3> steps;
};

// Upper bound on steps -- guards against a pathological spec allocating unboundedly.
inline constexpr std::uint32_t kMaxStairSteps = 1024;

// Pure + deterministic. Rejects (ok=false, empty steps, a reasonCode) any non-finite or
// non-positive dimension and a stepCount outside [1, kMaxStairSteps].
[[nodiscard]] StairMesh generateStairs(const StairSpec& spec);

}  // namespace iggy3d
