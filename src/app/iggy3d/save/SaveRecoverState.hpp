#pragma once

#include <string>

namespace iggy3d {

// Owned save-recover state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: save. Behavior-identical.
struct ProductSaveRecoverState {
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  bool executed = false;
  std::string saveId = "none";
  bool snapshotRecovered = false;
  bool snapshotMissing = false;
};

}  // namespace iggy3d
