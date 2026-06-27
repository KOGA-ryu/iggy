#pragma once

#include <cstdint>
#include <string>

#include "app/iggy3d/ProductRoomEditorCursor.hpp"
#include "app/iggy3d/ProductRoomGeometryOptimization.hpp"
#include "content/authoring/EditableRoomDocument.hpp"

namespace iggy3d {

struct ProductRoomEditorPlacementPreviewRequest {
  bool roomEditingReady = false;
  ProductRoomEditorCursorState cursor;
  const EditableRoomDocument* document = nullptr;
};

struct ProductRoomEditorPlacementPreviewResult {
  bool ok = false;
  std::string status = "room_editor_preview_not_requested";
  std::string reasonCode = "room_editor_preview_not_requested";
  std::string primitiveId = "none";
  bool candidateCommandReady = false;
  RoomEditCommand candidateCommand;
  std::string tool = "floor";
  std::string wallDirection = "up";
  std::int32_t gridX = 0;
  std::int32_t gridZ = 0;
  std::int32_t storyIndex = 0;
  Vec3 worldCenter;
  Vec3 floorSizeMeters;
  Vec3 wallStartMeters;
  Vec3 wallEndMeters;
  float wallBottomY = 0.0F;
  float wallHeightMeters = 0.0F;
  float wallThicknessMeters = 0.0F;
  ProductRoomGeometryOptimizationReport before;
  ProductRoomGeometryOptimizationReport after;
  std::uint64_t floorCountBefore = 0;
  std::uint64_t floorCountAfter = 0;
  std::uint64_t wallCountBefore = 0;
  std::uint64_t wallCountAfter = 0;
  std::int64_t optimizedDrawDelta = 0;
  std::int64_t optimizedTriangleDelta = 0;
  std::int64_t optimizedFloorRectDelta = 0;
  std::int64_t optimizedWallRunDelta = 0;
  bool wouldMerge = false;
};

ProductRoomEditorPlacementPreviewResult buildProductRoomEditorPlacementPreview(
    const ProductRoomEditorPlacementPreviewRequest& request);

}  // namespace iggy3d
