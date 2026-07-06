#pragma once

#include <string>

namespace iggy3d {

// Owned controller-action routing state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: input/ControllerActionRouting. Behavior-identical.
struct ProductControllerActionState {
  bool mapped = false;
  std::string status = "controller_action_not_requested";
  std::string reasonCode = "controller_action_not_requested";
  std::string control = "none";
  std::string mode = "player";
  std::string surface = "none";
  std::string inputAction = "none";
};

}  // namespace iggy3d
