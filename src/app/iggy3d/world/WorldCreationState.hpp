#pragma once

#include <string>

namespace iggy3d {

// Owned world-creation state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: world creation. Behavior-identical.
struct ProductWorldCreationState {
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  std::string worldId = "none";
  std::string worldTitle = "none";
  bool asciiRoomRequested = false;
  std::string asciiRoomId = "none";
  std::string asciiRoomSourceName = "none";
  bool initialSaveRequested = false;
  bool initialSaveWritten = false;
  std::string initialSaveId = "none";
  std::string initialSaveTitle = "none";
  std::string routeAfterCreate = "world_setup";
};

}  // namespace iggy3d
