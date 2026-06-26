#include "app/iggy3d/ProductRoomEditorOverlay.hpp"

#include <cmath>

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

}  // namespace iggy3d
