#pragma once

#include <cstdint>
#include <string>

namespace iggy3d {

// Owned state for the top-down map debug overlay -- the first cluster extracted out of the
// ProductAppWindowState god-struct (docs/appkernel_build_map_v0_1.md, L2). A new top-down-map field
// now lands HERE, in its domain, instead of adding a flat field to the 615-member struct. Populated
// by copyTopDownMapOverlay (ProjectionRefresh), serialized by ReceiptBuilder. Behavior-identical.
struct ProductTopDownMapState {
  bool visible = false;
  std::string purpose = "hidden";
  std::string size = "hidden";
  std::string status = "top_down_map_hidden";
  std::string reasonCode = "top_down_map_hidden";
  std::uint64_t itemCount = 0;
};

}  // namespace iggy3d
