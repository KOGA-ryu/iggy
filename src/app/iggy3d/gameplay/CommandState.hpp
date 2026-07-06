#pragma once

#include <string>

namespace iggy3d {

// Owned gameplay-command mirror state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: gameplay. Behavior-identical.
struct ProductGameplayCommandState {
  bool submitted = false;
  bool accepted = false;
  std::string kind = "none";
  std::string status = "not_requested";
};

}  // namespace iggy3d
