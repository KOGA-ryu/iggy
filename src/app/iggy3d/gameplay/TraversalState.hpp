#pragma once

#include <string>

namespace iggy3d {

// Owned traversal (parkour) state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: gameplay. Behavior-identical.
struct ProductGameplayTraversalState {
  bool requested = false;
  bool consumed = false;
  bool accepted = false;
  bool fallbackJumpAllowed = false;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  std::string mechanic = "none";
  std::string slotId = "none";
  std::string targetId = "none";
  std::string landingSurfaceId = "none";
  float startX = 0.0F;
  float startY = 0.0F;
  float startZ = 0.0F;
  float finalX = 0.0F;
  float finalY = 0.0F;
  float finalZ = 0.0F;
};

}  // namespace iggy3d
