#pragma once

#include <cstdint>
#include <string>

namespace iggy3d {

// Owned gameplay-target state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: gameplay. Behavior-identical.
struct ProductGameplayTargetState {
  std::string status = "not_requested";
  std::string action = "none";
  std::uint64_t entityId = 0;
  std::string stableName = "none";
  std::string kind = "none";
  float distanceMeters = 0.0F;
  bool supportsCommand = false;
};

}  // namespace iggy3d
