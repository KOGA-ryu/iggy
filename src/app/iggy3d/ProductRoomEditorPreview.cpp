#include "app/iggy3d/ProductRoomEditorPreview.hpp"

#include <cstdint>
#include <string>

namespace iggy3d {
namespace {

template <typename T>
std::int64_t signedDelta(T before, T after) {
  return static_cast<std::int64_t>(after) - static_cast<std::int64_t>(before);
}

ProductRoomEditorPlacementPreviewResult basePreviewResult(
    const ProductRoomEditorPlacementPreviewRequest& request,
    std::string status) {
  ProductRoomEditorPlacementPreviewResult result;
  result.status = std::move(status);
  result.reasonCode = result.status;
  result.tool = std::string(productRoomEditorToolName(request.cursor.selectedTool));
  result.wallDirection =
      std::string(productRoomEditorDirectionName(request.cursor.wallDirection));
  result.gridX = request.cursor.gridX;
  result.gridZ = request.cursor.gridZ;
  result.storyIndex = request.cursor.storyIndex;
  // branch-gate: BG-1045
  if (request.document != nullptr) {
    result.floorCountBefore =
        static_cast<std::uint64_t>(request.document->floors.size());
    result.wallCountBefore =
        static_cast<std::uint64_t>(request.document->walls.size());
    result.floorCountAfter = result.floorCountBefore;
    result.wallCountAfter = result.wallCountBefore;
    result.before = buildProductRoomGeometryOptimizationReport(request.document);
    result.after = result.before;
  }
  return result;
}

void copyCandidateFacts(ProductRoomEditorPlacementPreviewResult& result,
                        const RoomEditCommand& command) {
  // branch-gate: BG-1045
  switch (command.kind) {
    case RoomEditCommandKind::AddFloor:
      result.primitiveId = command.floor.id;
      result.worldCenter = command.floor.centerMeters;
      result.floorSizeMeters = command.floor.sizeMeters;
      break;
    case RoomEditCommandKind::AddWall:
      result.primitiveId = command.wall.id;
      result.wallStartMeters = command.wall.startMeters;
      result.wallEndMeters = command.wall.endMeters;
      result.wallBottomY = command.wall.bottomY;
      result.wallHeightMeters = command.wall.heightMeters;
      result.wallThicknessMeters = command.wall.thicknessMeters;
      result.worldCenter = {(command.wall.startMeters.x + command.wall.endMeters.x) *
                                0.5F,
                            command.wall.bottomY,
                            (command.wall.startMeters.z + command.wall.endMeters.z) *
                                0.5F};
      break;
    case RoomEditCommandKind::DeleteFloor:
    case RoomEditCommandKind::SetFloorSemantics:
    case RoomEditCommandKind::MoveFloor:
    case RoomEditCommandKind::ResizeFloor:
    case RoomEditCommandKind::DeleteWall:
    case RoomEditCommandKind::SetWallSemantics:
    case RoomEditCommandKind::MoveWall:
    case RoomEditCommandKind::StretchWall:
    case RoomEditCommandKind::RotateWall90:
    case RoomEditCommandKind::SetWallHeight:
    case RoomEditCommandKind::SetWallThickness:
      break;
  }
}

void fillOptimizationDeltas(ProductRoomEditorPlacementPreviewResult& result) {
  result.optimizedDrawDelta =
      signedDelta(result.before.optimizedDrawCount, result.after.optimizedDrawCount);
  result.optimizedTriangleDelta = signedDelta(result.before.optimizedTriangleCount,
                                              result.after.optimizedTriangleCount);
  result.optimizedFloorRectDelta = signedDelta(result.before.optimizedFloorRectCount,
                                               result.after.optimizedFloorRectCount);
  result.optimizedWallRunDelta = signedDelta(result.before.optimizedWallRunCount,
                                             result.after.optimizedWallRunCount);
  result.wouldMerge =
      (result.floorCountAfter > result.floorCountBefore &&
       result.optimizedFloorRectDelta == 0) ||
      (result.wallCountAfter > result.wallCountBefore &&
       result.optimizedWallRunDelta == 0);
}

}  // namespace

ProductRoomEditorPlacementPreviewResult buildProductRoomEditorPlacementPreview(
    const ProductRoomEditorPlacementPreviewRequest& request) {
  // branch-gate: BG-1045
  if (!request.roomEditingReady) {
    return basePreviewResult(request, "room_editor_preview_not_ready");
  }
  // branch-gate: BG-1045
  if (request.document == nullptr) {
    return basePreviewResult(request, "room_editor_preview_document_missing");
  }

  ProductRoomEditorPlacementPreviewResult result =
      basePreviewResult(request, "room_editor_preview_not_requested");
  const ProductRoomEditorCursorResult cursor =
      buildProductRoomEditorPlaceCommand(request.cursor, request.document);
  // branch-gate: BG-1045
  if (!cursor.ok || !cursor.command.has_value()) {
    result.status = cursor.status;
    result.reasonCode = cursor.reasonCode;
    return result;
  }

  EditableRoomDocument dryRun = *request.document;
  const RoomEditResult applied =
      applyRoomEditCommand(dryRun, *cursor.command);
  // branch-gate: BG-1045
  if (applied.status != RoomEditStatus::Applied) {
    result.status = "room_editor_preview_command_rejected";
    result.reasonCode = applied.reasonCode;
    // branch-gate: BG-1045
    result.primitiveId =
        applied.primitiveId.empty() ? std::string{"none"} : applied.primitiveId;
    return result;
  }

  result.ok = true;
  result.status = "room_editor_preview_ready";
  result.reasonCode = result.status;
  // branch-gate: BG-1045
  result.primitiveId =
      applied.primitiveId.empty() ? std::string{"none"} : applied.primitiveId;
  copyCandidateFacts(result, *cursor.command);
  result.floorCountAfter = static_cast<std::uint64_t>(dryRun.floors.size());
  result.wallCountAfter = static_cast<std::uint64_t>(dryRun.walls.size());
  result.after = buildProductRoomGeometryOptimizationReport(&dryRun);
  fillOptimizationDeltas(result);
  return result;
}

}  // namespace iggy3d
