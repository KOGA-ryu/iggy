#pragma once

#include <string>

namespace iggy3d {

// Owned jump-ability state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: gameplay. Behavior-identical.
struct ProductGameplayJumpState {
  bool requested = false;
  bool accepted = false;
  bool active = false;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  float velocityMetersPerSecond = 0.0F;
  float coyoteSecondsRemaining = 0.0F;
  float bufferSecondsRemaining = 0.0F;
  bool held = false;
  bool cutApplied = false;
  float groundY = 0.0F;
  float startY = 0.0F;
  float finalY = 0.0F;
  float heightMeters = 0.0F;
};

}  // namespace iggy3d
