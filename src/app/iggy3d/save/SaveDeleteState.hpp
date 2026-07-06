#pragma once

#include <string>

namespace iggy3d {

// Owned save-delete state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: save. Behavior-identical.
struct ProductSaveDeleteState {
  bool confirmationOpen = false;
  std::string candidateId = "none";
  bool candidateEnabled = false;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  std::string type = "none";
  bool recoverable = false;
  bool executed = false;
};

}  // namespace iggy3d
