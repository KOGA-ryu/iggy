#pragma once

#include <cstdint>
#include <string>

namespace iggy3d {

// Owned gameplay-outcome state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: gameplay. Behavior-identical.
struct ProductGameplayOutcomeState {
  std::string status = "not_requested";
  bool targetActiveAfter = false;
  bool inventoryChanged = false;
  std::string itemId = "none";
  std::uint64_t itemCount = 0;
  bool objectiveChanged = false;
  std::uint64_t eventCount = 0;
};

}  // namespace iggy3d
