#pragma once

#include <cstdint>
#include <string>

#include "app/iggy3d/ProductRoomEditorCursor.hpp"
#include "app/iggy3d/ProductRoomEditorPreview.hpp"
#include "core/math/Vec3.hpp"

namespace iggy3d {

struct ProductRoomEditorOverlay {
  bool visible = false;
  std::string status = "room_editor_overlay_not_ready";
  std::string reasonCode = "room_editor_overlay_not_ready";
  std::uint64_t itemCount = 0;
  ProductRoomEditorCursorState cursor;
  Vec3 worldPosition;
  Vec3 wallStartMeters;
  Vec3 wallEndMeters;
};

ProductRoomEditorOverlay buildProductRoomEditorOverlay(
    const ProductRoomEditorCursorState& cursor,
    bool roomEditingReady);

struct ProductRoomEditorPreviewOverlay {
  bool visible = false;
  std::string status = "room_editor_preview_overlay_not_ready";
  std::string reasonCode = "room_editor_preview_overlay_not_ready";
  std::uint64_t itemCount = 0;
  std::string candidateId = "none";
  ProductRoomEditorTool tool = ProductRoomEditorTool::Floor;
  std::string toolName = "floor";
  ProductRoomEditorDirection wallDirection = ProductRoomEditorDirection::Up;
  std::string wallDirectionName = "up";
  std::int32_t gridX = 0;
  std::int32_t gridZ = 0;
  std::int32_t storyIndex = 0;
  Vec3 worldPosition;
  Vec3 floorSizeMeters;
  Vec3 wallStartMeters;
  Vec3 wallEndMeters;
  float wallBottomY = 0.0F;
  float wallHeightMeters = 0.0F;
  float wallThicknessMeters = 0.0F;
  std::int64_t optimizedDrawDelta = 0;
  std::int64_t optimizedTriangleDelta = 0;
  std::int64_t optimizedFloorRectDelta = 0;
  std::int64_t optimizedWallRunDelta = 0;
  bool wouldMerge = false;
};

ProductRoomEditorPreviewOverlay buildProductRoomEditorPreviewOverlay(
    const ProductRoomEditorPlacementPreviewResult* preview);

}  // namespace iggy3d
