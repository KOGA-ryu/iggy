#pragma once

#include <cstdint>
#include <string>

#include "app/frontend/MenuInput.hpp"  // MenuOwner

namespace iggy3d {

// Owned automation-control state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: automation. Behavior-identical.
struct ProductAutomationControlState {
  bool requested = false;
  bool loaded = false;
  std::string path;
  std::string status = "not_requested";
  std::string scope = "none";
  std::uint64_t lineCount = 0;
  std::uint64_t appliedCount = 0;
  std::string lastKey = "none";
  std::string lastAction = "none";
  MenuOwner lastOwner = MenuOwner::None;
  std::string lastResult = "none";
};

}  // namespace iggy3d
