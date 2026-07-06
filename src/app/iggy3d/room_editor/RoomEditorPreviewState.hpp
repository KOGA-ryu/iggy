#pragma once

#include <cstdint>
#include <string>

namespace iggy3d {

// Owned room-editor placement-preview state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). A safe roomEditor SUB-prefix (distinct from the typed
// roomEditorPlacementPreview member, which stays flat). Domain: room_editor. Behavior-identical.
struct ProductRoomEditorPreviewState {
  bool active = false;
  bool visible = false;
  std::string status = "room_editor_preview_not_requested";
  std::string reasonCode = "room_editor_preview_not_requested";
  std::string candidateId = "none";
  std::string tool = "floor";
  std::int32_t gridX = 0;
  std::int32_t gridZ = 0;
  std::uint64_t beforeDrawCount = 0;
  std::uint64_t afterDrawCount = 0;
  std::int64_t avoidedDrawCountDelta = 0;
  std::uint64_t beforeTriangleCount = 0;
  std::uint64_t afterTriangleCount = 0;
  std::int64_t avoidedTriangleCountDelta = 0;
  std::int64_t optimizedDrawDelta = 0;
  std::int64_t optimizedTriangleDelta = 0;
};

}  // namespace iggy3d
