#pragma once

#include <cstdint>
#include <string>

namespace iggy3d {

// Owned NPC-behavior debug-HUD state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: debug. Behavior-identical.
struct ProductNpcBehaviorDebugHudState {
  bool visible = false;
  bool debugAvailable = false;
  std::uint64_t lineCount = 0;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  bool hasUnresolvedProfile = false;
};

}  // namespace iggy3d
