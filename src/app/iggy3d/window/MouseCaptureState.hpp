#pragma once

#include <string>

namespace iggy3d {

// Owned mouse-capture policy state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). A new mouse-capture field now lands here, in its domain
// (window/MouseCapturePolicy), not in the 600+-member struct. Behavior-identical.
struct ProductMouseCaptureState {
  bool requested = false;
  bool active = false;
  std::string status = "mouse_capture_not_requested";
  std::string reasonCode = "mouse_capture_gameplay_inactive";
  std::string mode = "none";
  std::string inputOwner = "none";
};

}  // namespace iggy3d
