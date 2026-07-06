#pragma once

#include <string>

namespace iggy3d {

// Owned wall-run gameplay state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: gameplay. Behavior-identical.
struct ProductWallRunState {
  bool candidateAvailable = false;
  std::string candidateStatus = "wall_run_not_checked";
  std::string candidateReasonCode = "wall_run_not_checked";
  std::string side = "none";
  std::string surfaceId = "none";
  float normalX = 0.0F;
  float normalY = 0.0F;
  float normalZ = 0.0F;
  float approachSpeedMetersPerSecond = 0.0F;
  bool active = false;
  std::string status = "wall_run_inactive";
  std::string reasonCode = "wall_run_inactive";
  float remainingSeconds = 0.0F;
  float durationSeconds = 0.0F;
  float gravityMultiplier = 1.0F;
  float speedMultiplier = 1.0F;
};

}  // namespace iggy3d
