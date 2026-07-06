#pragma once

#include <string>

namespace iggy3d {

// Owned gameplay-reset state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: gameplay. Behavior-identical.
struct ProductGameplayResetState {
  bool triggered = false;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  std::string spawnAnchorId = "none";
  std::string sourceAnchorId = "none";
  float startY = 0.0F;
  float finalY = 0.0F;
};

}  // namespace iggy3d
