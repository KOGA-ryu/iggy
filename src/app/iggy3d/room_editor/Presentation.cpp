#include "app/iggy3d/room_editor/Presentation.hpp"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>

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

  // branch-gate: BG-1046
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
  if (preview.tool == "object") {
    return ProductRoomEditorTool::Object;
  }
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

void appendHudLine(ProductRoomEditorHud& hud, std::string text) {
  // branch-gate: BG-1034
  if (hud.lineCount >= hud.lines.size()) {
    return;
  }
  ProductRoomEditorHudLine& line = hud.lines[hud.lineCount];
  line.visible = true;
  line.text = std::move(text);
  ++hud.lineCount;
}

std::string_view resultName(bool accepted) {
  constexpr std::array<std::string_view, 2> kResultNames = {
      "rejected",
      "accepted",
  };
  return kResultNames[accepted];
}

std::string signedNumber(std::int64_t value) {
  std::ostringstream out;
  out << std::showpos << value;
  return out.str();
}

std::string countImpactLine(std::string_view label,
                            std::uint64_t before,
                            std::uint64_t after,
                            std::int64_t delta) {
  return std::string(label) + " " + std::to_string(before) + " -> " +
         std::to_string(after) + " (" + signedNumber(delta) + ")";
}

}  // namespace

ProductRoomEditorOverlay buildProductRoomEditorOverlay(
    const ProductRoomEditorCursorState& cursor,
    bool roomEditingReady) {
  ProductRoomEditorOverlay overlay;
  overlay.cursor = cursor;
  // branch-gate: BG-1046
  if (!roomEditingReady) {
    return overlay;
  }
  // branch-gate: BG-1046
  if (!validCellSize(cursor.cellSizeMeters)) {
    overlay.status = "room_editor_overlay_invalid_cell_size";
    overlay.reasonCode = overlay.status;
    return overlay;
  }

  overlay.status = "room_editor_overlay_ready";
  overlay.reasonCode = overlay.status;
  overlay.visible = true;
  overlay.itemCount = 1;

  // branch-gate: BG-1046
  if (cursor.selectedTool == ProductRoomEditorTool::Wall) {
    // branch-gate: BG-1046
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
  overlay.objectSizeMeters = preview->objectSizeMeters;
  overlay.objectAssetId = preview->objectAssetId;
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

ProductRoomEditorHud buildProductRoomEditorHud(
    const ProductRoomEditorHudRequest& request) {
  ProductRoomEditorHud hud;
  hud.toolName = std::string(productRoomEditorToolName(request.cursor.selectedTool));
  hud.wallDirectionName =
      std::string(productRoomEditorDirectionName(request.cursor.wallDirection));
  hud.gridX = request.cursor.gridX;
  hud.gridZ = request.cursor.gridZ;
  hud.storyIndex = request.cursor.storyIndex;
  hud.lastOperation = std::string(request.lastOperation);
  hud.lastOperationAccepted = request.lastOperationAccepted;
  hud.lastPrimitiveId = std::string(request.lastPrimitiveId);
  // branch-gate: BG-1034
  if (request.placementPreview != nullptr) {
    hud.previewActive = true;
    hud.previewStatus = request.placementPreview->status;
    hud.previewCandidateId = request.placementPreview->primitiveId;
    hud.previewBeforeDrawCount =
        request.placementPreview->before.optimizedDrawCount;
    hud.previewAfterDrawCount =
        request.placementPreview->after.optimizedDrawCount;
    hud.previewBeforeTriangleCount =
        request.placementPreview->before.optimizedTriangleCount;
    hud.previewAfterTriangleCount =
        request.placementPreview->after.optimizedTriangleCount;
    hud.previewOptimizedDrawDelta =
        request.placementPreview->optimizedDrawDelta;
    hud.previewOptimizedTriangleDelta =
        request.placementPreview->optimizedTriangleDelta;
  }

  // branch-gate: BG-1034
  if (!request.gameplayActive || !request.roomEditing.ready) {
    return hud;
  }

  hud.visible = true;
  hud.status = "room_editor_hud_ready";
  hud.reasonCode = hud.status;
  appendHudLine(hud, "EDITOR TOOL " + hud.toolName);
  appendHudLine(hud, "GRID " + std::to_string(hud.gridX) + " " +
                         std::to_string(hud.gridZ) + " STORY " +
                         std::to_string(hud.storyIndex));
  // branch-gate: BG-1034
  if (request.cursor.selectedTool == ProductRoomEditorTool::Wall) {
    appendHudLine(hud, "WALL DIR " + hud.wallDirectionName);
  }
  // branch-gate: BG-1127
  if (request.cursor.selectedTool == ProductRoomEditorTool::Object) {
    appendHudLine(hud, "OBJECT " + request.cursor.selectedObjectAssetId);
  }
  // branch-gate: BG-1034
  if (hud.previewActive) {
    appendHudLine(hud, "PREVIEW " + hud.previewCandidateId + " " +
                           hud.previewStatus);
    appendHudLine(hud,
                  countImpactLine("DRAW",
                                  hud.previewBeforeDrawCount,
                                  hud.previewAfterDrawCount,
                                  hud.previewOptimizedDrawDelta));
    appendHudLine(hud,
                  countImpactLine("TRIS",
                                  hud.previewBeforeTriangleCount,
                                  hud.previewAfterTriangleCount,
                                  hud.previewOptimizedTriangleDelta));
    return hud;
  }
  appendHudLine(hud, "LAST " + hud.lastOperation + " " +
                         std::string(resultName(hud.lastOperationAccepted)));
  return hud;
}

}  // namespace iggy3d
