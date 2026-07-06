#pragma once

#include <cstdint>
#include <string>

namespace iggy3d {

// Owned save-flow state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: save. Behavior-identical.
struct ProductSaveFlowState {
  std::string operation = "none";
  std::string sourceSurface = "none";
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  std::string affectedSlotId = "none";
  std::uint64_t activeCountBefore = 0;
  std::uint64_t activeCountAfter = 0;
  std::uint64_t deletedCountAfter = 0;
  std::string selectedSlotAfter = "none";
};

}  // namespace iggy3d
