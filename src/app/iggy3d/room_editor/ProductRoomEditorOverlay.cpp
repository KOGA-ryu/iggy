#include "app/iggy3d/room_editor/ProductRoomEditorOverlay.hpp"

#include <cmath>
#include <string>

namespace iggy3d {
namespace {

constexpr float kCursorFloorY = 0.08F;
constexpr float kCursorWallY = 0.15F;

bool validCellSize(float cellSizeMeters) {
  return std::isfinite(cellSizeMeters) && cellSizeMeters > 0.0F;
}

Vec3 floorCursorCenter(const ProductRoomEditorCursorState& cursor) {
  return {static_cast<float>(cursor.gridX) * cursor.cellSizeMeters,
          kCursorFloorY,
          static_cast<float>(cursor.gridZ) * cursor.cellSizeMeters};
}

bool fillWallEdge(const ProductRoomEditorCursorState& cursor,
                  Vec3& startMeters,
                  Vec3& endMeters) {
  const float half = cursor.cellSizeMeters * 0.5F;
  const float centerX = static_cast<float>(cursor.gridX) * cursor.cellSizeMeters;
  const float centerZ = static_cast<float>(cursor.gridZ) * cursor.cellSizeMeters;
  const float west = centerX - half;
  const float east = centerX + half;
  const float north = centerZ - half;
  const float south = centerZ + half;

  switch (cursor.wallDirection) {
    case ProductRoomEditorDirection::Up:
      startMeters = {west, 0.0F, north};
      endMeters = {east, 0.0F, north};
      return true;
    case ProductRoomEditorDirection::Right:
      startMeters = {east, 0.0F, north};
      endMeters = {east, 0.0F, south};
      return true;
    case ProductRoomEditorDirection::Down:
      startMeters = {east, 0.0F, south};
      endMeters = {west, 0.0F, south};
      return true;
    case ProductRoomEditorDirection::Left:
      startMeters = {west, 0.0F, south};
      endMeters = {west, 0.0F, north};
      return true;
  }
  return false;
}

Vec3 midpoint(Vec3 a, Vec3 b) {
  return {(a.x + b.x) * 0.5F, kCursorWallY, (a.z + b.z) * 0.5F};
}

ProductRoomEditorTool toolFromPreview(const ProductRoomEditorPlacementPreviewResult& preview) {
  // branch-gate: BG-1046
  if (preview.tool == "wall") {
    return ProductRoomEditorTool::Wall;
  }
  return ProductRoomEditorTool::Floor;
}

ProductRoomEditorDirection wallDirectionFromPreview(
    const ProductRoomEditorPlacementPreviewResult& preview) {
  // branch-gate: BG-1046
  if (preview.wallDirection == "right") {
    return ProductRoomEditorDirection::Right;
  }
  // branch-gate: BG-1046
  if (preview.wallDirection == "down") {
    return ProductRoomEditorDirection::Down;
  }
  // branch-gate: BG-1046
  if (preview.wallDirection == "left") {
    return ProductRoomEditorDirection::Left;
  }
  return ProductRoomEditorDirection::Up;
}

}  // namespace

ProductRoomEditorOverlay buildProductRoomEditorOverlay(
    const ProductRoomEditorCursorState& cursor,
    bool roomEditingReady) {
  ProductRoomEditorOverlay overlay;
  overlay.cursor = cursor;
  if (!roomEditingReady) {
    return overlay;
  }
  if (!validCellSize(cursor.cellSizeMeters)) {
    overlay.status = "room_editor_overlay_invalid_cell_size";
    overlay.reasonCode = overlay.status;
    return overlay;
  }

  overlay.status = "room_editor_overlay_ready";
  overlay.reasonCode = overlay.status;
  overlay.visible = true;
  overlay.itemCount = 1;

  if (cursor.selectedTool == ProductRoomEditorTool::Wall) {
    if (!fillWallEdge(cursor, overlay.wallStartMeters, overlay.wallEndMeters)) {
      overlay.visible = false;
      overlay.itemCount = 0;
      overlay.status = "room_editor_overlay_invalid_direction";
      overlay.reasonCode = overlay.status;
      return overlay;
    }
    overlay.worldPosition =
        midpoint(overlay.wallStartMeters, overlay.wallEndMeters);
    return overlay;
  }

  overlay.worldPosition = floorCursorCenter(cursor);
  return overlay;
}

ProductRoomEditorPreviewOverlay buildProductRoomEditorPreviewOverlay(
    const ProductRoomEditorPlacementPreviewResult* preview) {
  ProductRoomEditorPreviewOverlay overlay;
  // branch-gate: BG-1046
  if (preview == nullptr) {
    overlay.status = "room_editor_preview_overlay_not_requested";
    overlay.reasonCode = overlay.status;
    return overlay;
  }

  overlay.candidateId = preview->primitiveId;
  overlay.tool = toolFromPreview(*preview);
  overlay.toolName = preview->tool;
  overlay.wallDirection = wallDirectionFromPreview(*preview);
  overlay.wallDirectionName = preview->wallDirection;
  overlay.gridX = preview->gridX;
  overlay.gridZ = preview->gridZ;
  overlay.storyIndex = preview->storyIndex;
  overlay.worldPosition = preview->worldCenter;
  overlay.floorSizeMeters = preview->floorSizeMeters;
  overlay.wallStartMeters = preview->wallStartMeters;
  overlay.wallEndMeters = preview->wallEndMeters;
  overlay.wallBottomY = preview->wallBottomY;
  overlay.wallHeightMeters = preview->wallHeightMeters;
  overlay.wallThicknessMeters = preview->wallThicknessMeters;
  overlay.optimizedDrawDelta = preview->optimizedDrawDelta;
  overlay.optimizedTriangleDelta = preview->optimizedTriangleDelta;
  overlay.optimizedFloorRectDelta = preview->optimizedFloorRectDelta;
  overlay.optimizedWallRunDelta = preview->optimizedWallRunDelta;
  overlay.wouldMerge = preview->wouldMerge;

  // branch-gate: BG-1046
  if (!preview->ok) {
    overlay.status = preview->status;
    overlay.reasonCode = preview->reasonCode;
    return overlay;
  }

  overlay.visible = true;
  overlay.status = "room_editor_preview_overlay_ready";
  overlay.reasonCode = overlay.status;
  overlay.itemCount = 1;
  return overlay;
}

}  // namespace iggy3d
