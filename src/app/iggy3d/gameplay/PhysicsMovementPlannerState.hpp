#pragma once

#include <string>

namespace iggy3d {

// Owned physics-movement-planner mirror state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: gameplay. Behavior-identical.
struct ProductPhysicsMovementPlannerState {
  bool enabled = false;
  bool requested = false;
  bool used = false;
  std::string status = "physics_movement_planner_disabled";
  std::string reasonCode = "physics_movement_planner_disabled";
};

}  // namespace iggy3d
