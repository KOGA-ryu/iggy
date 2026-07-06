#pragma once

#include <string>

namespace iggy3d {

// Owned dev collision-overlay state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: debug. Behavior-identical.
struct ProductDevCollisionOverlayState {
  bool visible = false;
  std::string status = "dev_collision_overlay_hidden";
  std::string reasonCode = "dev_collision_overlay_hidden";
};

}  // namespace iggy3d
