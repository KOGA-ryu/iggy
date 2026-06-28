#include "app/iggy3d/room_editor/Preview.hpp"
#include "app/iggy3d/room/GeometryOptimization.hpp"

#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::EditableRoomFloor floor(std::string id, float x, float z) {
  iggy3d::EditableRoomFloor out;
  out.id = std::move(id);
  out.storyIndex = 0;
  out.centerMeters = {x, -0.05F, z};
  out.sizeMeters = {1.0F, 0.10F, 1.0F};
  out.semantics = iggy3d::defaultFloorSemantics();
  return out;
}

iggy3d::EditableRoomWall wall(std::string id,
                              float startX,
                              float startZ,
                              float endX,
                              float endZ) {
  iggy3d::EditableRoomWall out;
  out.id = std::move(id);
  out.storyIndex = 0;
  out.startMeters = {startX, 0.0F, startZ};
  out.endMeters = {endX, 0.0F, endZ};
  out.bottomY = 0.0F;
  out.heightMeters = 2.5F;
  out.thicknessMeters = 1.0F;
  out.semantics = iggy3d::defaultWallSemantics();
  return out;
}

iggy3d::ProductRoomEditorPlacementPreviewRequest previewRequest(
    iggy3d::EditableRoomDocument& document,
    iggy3d::ProductRoomEditorCursorState cursor) {
  iggy3d::ProductRoomEditorPlacementPreviewRequest request;
  request.roomEditingReady = true;
  request.cursor = cursor;
  request.document = &document;
  return request;
}

bool missingAndNotReadyRejectWithoutMutation() {
  iggy3d::EditableRoomDocument document;
  document.floors.push_back(floor("floor_1", 0.0F, 0.0F));
  iggy3d::ProductRoomEditorCursorState cursor;

  iggy3d::ProductRoomEditorPlacementPreviewRequest request;
  request.roomEditingReady = false;
  request.cursor = cursor;
  request.document = &document;
  const iggy3d::ProductRoomEditorPlacementPreviewResult notReady =
      iggy3d::buildProductRoomEditorPlacementPreview(request);

  request.roomEditingReady = true;
  request.document = nullptr;
  const iggy3d::ProductRoomEditorPlacementPreviewResult missing =
      iggy3d::buildProductRoomEditorPlacementPreview(request);

  return expect(!notReady.ok, "not ready rejected") &&
         expect(notReady.status == "room_editor_preview_not_ready",
                "not ready status") &&
         expect(!missing.ok, "missing document rejected") &&
         expect(missing.status == "room_editor_preview_document_missing",
                "missing document status") &&
         expect(document.floors.size() == 1U, "reject leaves floor count") &&
         expect(document.walls.empty(), "reject leaves wall count");
}

bool invalidCursorPropagatesCursorReason() {
  iggy3d::EditableRoomDocument document;
  iggy3d::ProductRoomEditorCursorState cursor;
  cursor.cellSizeMeters = 0.0F;

  const iggy3d::ProductRoomEditorPlacementPreviewResult result =
      iggy3d::buildProductRoomEditorPlacementPreview(
          previewRequest(document, cursor));
  return expect(!result.ok, "invalid cursor rejected") &&
         expect(result.status == "room_editor_invalid_cell_size",
                "invalid cursor status") &&
         expect(document.floors.empty(), "invalid cursor leaves document");
}

bool floorPreviewDeltasDistinguishMergeAndIsolation() {
  iggy3d::EditableRoomDocument adjacentDocument;
  adjacentDocument.floors.push_back(floor("floor_1", 0.0F, 0.0F));
  iggy3d::ProductRoomEditorCursorState cursor;
  cursor.selectedTool = iggy3d::ProductRoomEditorTool::Floor;
  cursor.gridX = 1;
  const iggy3d::ProductRoomEditorPlacementPreviewResult adjacent =
      iggy3d::buildProductRoomEditorPlacementPreview(
          previewRequest(adjacentDocument, cursor));

  iggy3d::EditableRoomDocument isolatedDocument;
  isolatedDocument.floors.push_back(floor("floor_1", 0.0F, 0.0F));
  cursor.gridX = 2;
  const iggy3d::ProductRoomEditorPlacementPreviewResult isolated =
      iggy3d::buildProductRoomEditorPlacementPreview(
          previewRequest(isolatedDocument, cursor));

  return expect(adjacent.ok, "adjacent floor preview accepted") &&
         expect(adjacent.primitiveId == "edit_floor_1",
                "adjacent floor primitive id") &&
         expect(adjacent.candidateCommandReady,
                "adjacent floor candidate command ready") &&
         expect(adjacent.candidateCommand.kind ==
                    iggy3d::RoomEditCommandKind::AddFloor,
                "adjacent floor candidate command kind") &&
         expect(adjacent.floorCountBefore == 1U, "adjacent floor before count") &&
         expect(adjacent.floorCountAfter == 2U, "adjacent floor after count") &&
         expect(adjacent.before.optimizedDrawCount == 1U,
                "adjacent floor before optimized draw count") &&
         expect(adjacent.after.optimizedDrawCount == 1U,
                "adjacent floor after optimized draw count") &&
         expect(adjacent.before.optimizedTriangleCount == 2U,
                "adjacent floor before optimized triangle count") &&
         expect(adjacent.after.optimizedTriangleCount == 2U,
                "adjacent floor after optimized triangle count") &&
         expect(adjacent.after.drawCountAvoided -
                    adjacent.before.drawCountAvoided == 1U,
                "adjacent floor avoided draw delta") &&
         expect(adjacent.after.triangleCountAvoided -
                    adjacent.before.triangleCountAvoided == 2U,
                "adjacent floor avoided triangle delta") &&
         expect(adjacent.floorSizeMeters.x == 1.0F &&
                    adjacent.floorSizeMeters.z == 1.0F,
                "adjacent floor size facts") &&
         expect(adjacent.optimizedFloorRectDelta == 0,
                "adjacent floor rect merges") &&
         expect(adjacent.optimizedDrawDelta == 0,
                "adjacent floor draw delta merged") &&
         expect(adjacent.optimizedTriangleDelta == 0,
                "adjacent floor triangle delta merged") &&
         expect(adjacent.wouldMerge, "adjacent floor would merge") &&
         expect(adjacentDocument.floors.size() == 1U,
                "adjacent preview leaves document") &&
         expect(isolated.ok, "isolated floor preview accepted") &&
         expect(isolated.before.optimizedDrawCount == 1U,
                "isolated floor before optimized draw count") &&
         expect(isolated.after.optimizedDrawCount == 2U,
                "isolated floor after optimized draw count") &&
         expect(isolated.before.optimizedTriangleCount == 2U,
                "isolated floor before optimized triangle count") &&
         expect(isolated.after.optimizedTriangleCount == 4U,
                "isolated floor after optimized triangle count") &&
         expect(isolated.optimizedFloorRectDelta == 1,
                "isolated floor rect delta") &&
         expect(isolated.optimizedDrawDelta == 1,
                "isolated floor draw delta") &&
         expect(isolated.optimizedTriangleDelta == 2,
                "isolated floor triangle delta") &&
         expect(!isolated.wouldMerge, "isolated floor does not merge") &&
         expect(isolatedDocument.floors.size() == 1U,
                "isolated preview leaves document");
}

bool wallPreviewDeltasDistinguishMergeAndIsolation() {
  iggy3d::EditableRoomDocument collinearDocument;
  collinearDocument.walls.push_back(wall("wall_1", -0.5F, -0.5F, 0.5F, -0.5F));
  iggy3d::ProductRoomEditorCursorState cursor;
  cursor.selectedTool = iggy3d::ProductRoomEditorTool::Wall;
  cursor.wallDirection = iggy3d::ProductRoomEditorDirection::Up;
  cursor.gridX = 1;
  const iggy3d::ProductRoomEditorPlacementPreviewResult collinear =
      iggy3d::buildProductRoomEditorPlacementPreview(
          previewRequest(collinearDocument, cursor));

  iggy3d::EditableRoomDocument isolatedDocument;
  isolatedDocument.walls.push_back(wall("wall_1", -0.5F, -0.5F, 0.5F, -0.5F));
  cursor.gridX = 3;
  const iggy3d::ProductRoomEditorPlacementPreviewResult isolated =
      iggy3d::buildProductRoomEditorPlacementPreview(
          previewRequest(isolatedDocument, cursor));
  iggy3d::EditableRoomDocument isolatedDryRun = isolatedDocument;
  const iggy3d::RoomEditResult isolatedApplied =
      iggy3d::applyRoomEditCommand(isolatedDryRun, isolated.candidateCommand);
  const iggy3d::ProductRoomGeometryOptimizationReport isolatedDryRunReport =
      iggy3d::buildProductRoomGeometryOptimizationReport(&isolatedDryRun);

  return expect(collinear.ok, "collinear wall preview accepted") &&
         expect(collinear.primitiveId == "edit_wall_1",
                "collinear wall primitive id") &&
         expect(collinear.wallCountBefore == 1U, "collinear wall before count") &&
         expect(collinear.wallCountAfter == 2U, "collinear wall after count") &&
         expect(collinear.before.optimizedDrawCount == 1U,
                "collinear wall before optimized draw count") &&
         expect(collinear.after.optimizedDrawCount == 1U,
                "collinear wall after optimized draw count") &&
         expect(collinear.before.optimizedTriangleCount == 12U,
                "collinear wall before optimized triangle count") &&
         expect(collinear.after.optimizedTriangleCount == 12U,
                "collinear wall after optimized triangle count") &&
         expect(collinear.after.drawCountAvoided -
                    collinear.before.drawCountAvoided == 1U,
                "collinear wall avoided draw delta") &&
         expect(collinear.after.triangleCountAvoided -
                    collinear.before.triangleCountAvoided == 12U,
                "collinear wall avoided triangle delta") &&
         expect(collinear.optimizedWallRunDelta == 0,
                "collinear wall run merges") &&
         expect(collinear.optimizedDrawDelta == 0,
                "collinear wall draw delta merged") &&
         expect(collinear.optimizedTriangleDelta == 0,
                "collinear wall triangle delta merged") &&
         expect(collinear.wouldMerge, "collinear wall would merge") &&
         expect(collinearDocument.walls.size() == 1U,
                "collinear preview leaves document") &&
         expect(isolated.ok, "isolated wall preview accepted") &&
         expect(isolatedApplied.status == iggy3d::RoomEditStatus::Applied,
                "isolated wall dry-run applies") &&
         expect(isolated.after.optimizedDrawCount ==
                    isolatedDryRunReport.optimizedDrawCount,
                "isolated wall after draw matches dry-run") &&
         expect(isolated.after.optimizedTriangleCount ==
                    isolatedDryRunReport.optimizedTriangleCount,
                "isolated wall after triangles match dry-run") &&
         expect(isolated.before.optimizedDrawCount == 1U,
                "isolated wall before optimized draw count") &&
         expect(isolated.after.optimizedDrawCount == 2U,
                "isolated wall after optimized draw count") &&
         expect(isolated.before.optimizedTriangleCount == 12U,
                "isolated wall before optimized triangle count") &&
         expect(isolated.after.optimizedTriangleCount == 24U,
                "isolated wall after optimized triangle count") &&
         expect(isolated.optimizedWallRunDelta == 1,
                "isolated wall run delta") &&
         expect(isolated.optimizedDrawDelta == 1,
                "isolated wall draw delta") &&
         expect(isolated.optimizedTriangleDelta == 12,
                "isolated wall triangle delta") &&
         expect(!isolated.wouldMerge, "isolated wall does not merge") &&
         expect(isolatedDocument.walls.size() == 1U,
                "isolated wall preview leaves document");
}

bool wallDirectionAndCandidateFactsAreStable() {
  iggy3d::EditableRoomDocument document;
  iggy3d::ProductRoomEditorCursorState cursor;
  cursor.selectedTool = iggy3d::ProductRoomEditorTool::Wall;
  cursor.wallDirection = iggy3d::ProductRoomEditorDirection::Right;
  cursor.gridX = 2;
  cursor.gridZ = 3;
  const iggy3d::ProductRoomEditorPlacementPreviewResult result =
      iggy3d::buildProductRoomEditorPlacementPreview(
          previewRequest(document, cursor));

  return expect(result.ok, "direction preview accepted") &&
         expect(result.tool == "wall", "direction preview tool") &&
         expect(result.wallDirection == "right",
                "direction preview wall direction") &&
         expect(result.primitiveId == "edit_wall_1",
                "direction preview primitive id") &&
         expect(result.candidateCommandReady,
                "direction preview candidate command ready") &&
         expect(result.candidateCommand.kind ==
                    iggy3d::RoomEditCommandKind::AddWall,
                "direction preview candidate command kind") &&
         expect(result.gridX == 2 && result.gridZ == 3,
                "direction preview grid") &&
         expect(result.wallStartMeters.x == 2.5F,
                "right wall start x") &&
         expect(result.wallStartMeters.z == 2.5F,
                "right wall start z") &&
         expect(result.wallEndMeters.x == 2.5F,
                "right wall end x") &&
         expect(result.wallEndMeters.z == 3.5F,
                "right wall end z") &&
         expect(result.wallBottomY == 0.0F, "right wall bottom") &&
         expect(result.wallHeightMeters == 2.5F, "right wall height") &&
         expect(result.wallThicknessMeters == 1.0F, "right wall thickness") &&
         expect(document.walls.empty(), "candidate facts leave document");
}

bool objectPreviewDryRunsWithoutDocumentMutation() {
  iggy3d::EditableRoomDocument document;
  document.floors.push_back(floor("floor_1", 0.0F, 0.0F));
  iggy3d::ProductRoomEditorCursorState cursor;
  cursor.selectedTool = iggy3d::ProductRoomEditorTool::Object;
  cursor.gridX = 2;
  cursor.gridZ = -1;

  const iggy3d::ProductRoomEditorPlacementPreviewResult result =
      iggy3d::buildProductRoomEditorPlacementPreview(
          previewRequest(document, cursor));
  iggy3d::EditableRoomDocument dryRun = document;
  const iggy3d::RoomEditResult applied =
      iggy3d::applyRoomEditCommand(dryRun, result.candidateCommand);
  const iggy3d::ProductRoomGeometryOptimizationReport dryRunReport =
      iggy3d::buildProductRoomGeometryOptimizationReport(&dryRun);

  return expect(result.ok, "object preview accepted") &&
         expect(result.tool == "object", "object preview tool") &&
         expect(result.primitiveId == "edit_object_1",
                "object preview primitive id") &&
         expect(result.candidateCommandReady, "object candidate ready") &&
         expect(result.candidateCommand.kind ==
                    iggy3d::RoomEditCommandKind::AddObject,
                "object candidate command kind") &&
         expect(result.candidateCommand.object.assetId == "wood_crate_proxy",
                "object candidate asset id") &&
         expect(result.objectAssetId == "wood_crate_proxy",
                "object preview asset id") &&
         expect(result.objectCountBefore == 0U, "object before count") &&
         expect(result.objectCountAfter == 1U, "object after count") &&
         expect(result.before.optimizedDrawCount == 1U,
                "object before optimized draw count") &&
         expect(result.after.optimizedDrawCount == 2U,
                "object after optimized draw count") &&
         expect(result.before.optimizedTriangleCount == 2U,
                "object before optimized triangle count") &&
         expect(result.after.optimizedTriangleCount == 14U,
                "object after optimized triangle count") &&
         expect(result.optimizedDrawDelta == 1,
                "object optimized draw delta") &&
         expect(result.optimizedTriangleDelta == 12,
                "object optimized triangle delta") &&
         expect(result.objectSizeMeters.x == 0.8F &&
                    result.objectSizeMeters.y == 0.8F &&
                    result.objectSizeMeters.z == 0.8F,
                "object preview size") &&
         expect(result.worldCenter.x == 2.0F && result.worldCenter.y == 0.4F &&
                    result.worldCenter.z == -1.0F,
                "object preview world center") &&
         expect(document.objects.empty(), "object preview leaves document") &&
         expect(applied.status == iggy3d::RoomEditStatus::Applied,
                "object dry-run applies") &&
         expect(dryRun.objects.size() == 1U, "object dry-run mutates copy") &&
         expect(result.after.optimizedDrawCount ==
                    dryRunReport.optimizedDrawCount,
                "object after draw matches dry-run") &&
         expect(result.after.optimizedTriangleCount ==
                    dryRunReport.optimizedTriangleCount,
                "object after triangles matches dry-run");
}

bool duplicateCandidateRejectsWithEditReason() {
  iggy3d::EditableRoomDocument document;
  document.floors.push_back(floor("edit_floor_0", 0.0F, 0.0F));
  document.floors.push_back(
      floor("edit_floor_" +
                std::to_string(std::numeric_limits<std::uint64_t>::max()),
            1.0F,
            0.0F));
  iggy3d::ProductRoomEditorCursorState cursor;
  cursor.selectedTool = iggy3d::ProductRoomEditorTool::Floor;
  cursor.gridX = 2;
  const iggy3d::ProductRoomEditorPlacementPreviewResult result =
      iggy3d::buildProductRoomEditorPlacementPreview(
          previewRequest(document, cursor));

  return expect(!result.ok, "duplicate candidate rejected") &&
         expect(result.status == "room_editor_preview_command_rejected",
                "duplicate candidate status") &&
         expect(result.reasonCode == "room_edit_duplicate_id",
                "duplicate candidate reason") &&
         expect(result.primitiveId == "edit_floor_0",
                "duplicate candidate primitive") &&
         expect(document.floors.size() == 2U,
                "duplicate preview leaves document");
}

}  // namespace

int main() {
  const bool ok = missingAndNotReadyRejectWithoutMutation() &&
                  invalidCursorPropagatesCursorReason() &&
                  floorPreviewDeltasDistinguishMergeAndIsolation() &&
                  wallPreviewDeltasDistinguishMergeAndIsolation() &&
                  wallDirectionAndCandidateFactsAreStable() &&
                  objectPreviewDryRunsWithoutDocumentMutation() &&
                  duplicateCandidateRejectsWithEditReason();
  return ok ? 0 : 1;
}
