#pragma once

#include <string>

namespace iggy3d {

// Owned ascii-room draft state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: ascii_room. Behavior-identical.
struct ProductAsciiRoomDraftState {
  std::string text;
  std::string roomId = "ascii_preview";
  std::string sourceName = "automation_ascii_room";
};

}  // namespace iggy3d
