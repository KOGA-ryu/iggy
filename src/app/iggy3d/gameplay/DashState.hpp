#pragma once

#include <string>

namespace iggy3d {

// Owned dash-ability state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: gameplay. Behavior-identical.
struct ProductGameplayDashState {
  bool requested = false;
  bool accepted = false;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  float speedMetersPerSecond = 0.0F;
  float distanceMeters = 0.0F;
  float cooldownRemainingSeconds = 0.0F;
  float directionX = 0.0F;
  float directionZ = 0.0F;
};

}  // namespace iggy3d
