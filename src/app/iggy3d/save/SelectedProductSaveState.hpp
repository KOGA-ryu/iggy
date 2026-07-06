#pragma once

#include <string>

namespace iggy3d {

// Owned selected-product-save state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: save. Behavior-identical.
struct ProductSelectedProductSaveState {
  std::string id = "none";
  bool enabled = false;
  std::string status = "none";
};

}  // namespace iggy3d
