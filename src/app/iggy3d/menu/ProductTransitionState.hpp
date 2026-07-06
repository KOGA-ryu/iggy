#pragma once

#include <string>

namespace iggy3d {

// Owned menu/gameplay transition state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: menu transitions. Behavior-identical.
struct ProductTransitionState {
  std::string lastAction = "none";
  std::string status = "not_requested";
  bool returnedToGameplay = false;
  bool returnedToTitle = false;
  bool sessionPreserved = false;
};

}  // namespace iggy3d
