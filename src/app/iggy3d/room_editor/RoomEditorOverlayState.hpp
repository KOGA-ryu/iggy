#pragma once

#include <cstdint>
#include <string>

namespace iggy3d {

// Owned room-editor overlay state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). One of the safe roomEditor SUB-prefixes; the bare
// roomEditor core (+ its typed members roomEditorCursor/PlacementPreview/Hud) stays flat because
// the bare prefix collides with them. Domain: room_editor. Behavior-identical.
struct ProductRoomEditorOverlayState {
  bool visible = false;
  std::string status = "room_editor_overlay_not_ready";
  std::string reasonCode = "room_editor_overlay_not_ready";
  std::uint64_t itemCount = 0;
  float worldX = 0.0F;
  float worldY = 0.0F;
  float worldZ = 0.0F;
};

}  // namespace iggy3d
