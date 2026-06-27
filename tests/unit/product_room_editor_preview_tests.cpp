#include "app/iggy3d/room_editor/Preview.hpp"

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

  return expect(collinear.ok, "collinear wall preview accepted") &&
         expect(collinear.primitiveId == "edit_wall_1",
                "collinear wall primitive id") &&
         expect(collinear.wallCountBefore == 1U, "collinear wall before count") &&
         expect(collinear.wallCountAfter == 2U, "collinear wall after count") &&
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
                  duplicateCandidateRejectsWithEditReason();
  return ok ? 0 : 1;
}
