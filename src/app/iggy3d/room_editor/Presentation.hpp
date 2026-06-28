#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

#include "app/iggy3d/room_editor/EditingState.hpp"
#include "app/iggy3d/room_editor/Cursor.hpp"
#include "app/iggy3d/room_editor/Preview.hpp"
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
  Vec3 objectSizeMeters;
  std::string objectAssetId = "none";
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

struct ProductRoomEditorHudLine {
  bool visible = false;
  std::string text;
};

struct ProductRoomEditorHud {
  bool visible = false;
  std::string status = "room_editor_hud_not_ready";
  std::string reasonCode = "room_editor_hud_not_ready";
  std::string toolName = "floor";
  std::string wallDirectionName = "up";
  std::int32_t gridX = 0;
  std::int32_t gridZ = 0;
  std::int32_t storyIndex = 0;
  std::string lastOperation = "none";
  bool lastOperationAccepted = false;
  std::string lastPrimitiveId = "none";
  bool previewActive = false;
  std::string previewStatus = "room_editor_preview_not_requested";
  std::string previewCandidateId = "none";
  std::uint64_t previewBeforeDrawCount = 0;
  std::uint64_t previewAfterDrawCount = 0;
  std::uint64_t previewBeforeTriangleCount = 0;
  std::uint64_t previewAfterTriangleCount = 0;
  std::int64_t previewOptimizedDrawDelta = 0;
  std::int64_t previewOptimizedTriangleDelta = 0;
  std::array<ProductRoomEditorHudLine, 6> lines;
  std::uint64_t lineCount = 0;
};

struct ProductRoomEditorHudRequest {
  const ProductRoomEditingState& roomEditing;
  const ProductRoomEditorCursorState& cursor;
  bool gameplayActive = false;
  std::string_view lastOperation = "none";
  bool lastOperationAccepted = false;
  std::string_view lastPrimitiveId = "none";
  const ProductRoomEditorPlacementPreviewResult* placementPreview = nullptr;
};

ProductRoomEditorOverlay buildProductRoomEditorOverlay(
    const ProductRoomEditorCursorState& cursor,
    bool roomEditingReady);

ProductRoomEditorPreviewOverlay buildProductRoomEditorPreviewOverlay(
    const ProductRoomEditorPlacementPreviewResult* preview);

ProductRoomEditorHud buildProductRoomEditorHud(
    const ProductRoomEditorHudRequest& request);

}  // namespace iggy3d
