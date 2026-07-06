#pragma once

#include <string>

namespace iggy3d {

// Owned controller mode-toggle state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: input/InteractionModeState. Behavior-identical.
struct ProductControllerModeToggleState {
  bool requested = false;
  bool accepted = false;
  std::string status = "interaction_mode_toggle_not_requested";
  std::string reasonCode = "interaction_mode_toggle_not_requested";
  std::string surface = "none";
};

}  // namespace iggy3d
