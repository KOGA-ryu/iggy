#pragma once

#include <cstdint>
#include <string>

namespace iggy3d {

// Owned world-setup screen state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: world setup. Behavior-identical.
struct ProductWorldSetupState {
  std::string title = "New World";
  std::string status = "not_requested";
  std::string dungeonTitle = "none";
  std::uint64_t dungeonIndex = 0;
  std::uint64_t dungeonCount = 0;
  bool asciiRoomEnabled = false;
  bool asciiRoomTextPresent = false;
  std::string asciiRoomId = "world_setup_room";
  std::string asciiRoomSourceName = "world_setup_ascii_room.iggyroom.txt";
  bool dungeonDraftEditMode = false;
  bool dungeonDraftModified = false;
  std::uint64_t dungeonDraftCursorRow = 0;
  std::uint64_t dungeonDraftCursorColumn = 0;
  std::string dungeonDraftStatus = "not_requested";
  std::string dungeonDraftReasonCode = "not_requested";
  std::string dungeonDraftSelectedGlyph = ".";
  std::string dungeonDraftLastGlyph = "none";
};

}  // namespace iggy3d
