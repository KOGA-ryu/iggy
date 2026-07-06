#pragma once

#include <cstdint>

namespace iggy3d {

// Owned gameplay-collision-usage state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: gameplay. Behavior-identical.
struct ProductGameplayCollisionState {
  bool surfacesUsed = false;
  std::uint64_t surfaceCount = 0;
};

}  // namespace iggy3d
