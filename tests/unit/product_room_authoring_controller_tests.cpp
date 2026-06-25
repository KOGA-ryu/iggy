#include "app/iggy3d/AsciiRoomGrid.hpp"
#include "app/iggy3d/AsciiRoomSource.hpp"
#include "app/iggy3d/AsciiRoomToEditableRoom.hpp"
#include "app/iggy3d/ProductRoomAuthoringController.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

iggy3d::AsciiRoomCompileConfig compileConfig() {
  iggy3d::AsciiRoomCompileConfig config;
  config.roomId = "controller_ascii_room";
  config.sourceName = "tests/product_room_authoring_controller.iggyroom.txt";
  config.tileSizeMeters = 1.0F;
  config.floorThicknessMeters = 0.10F;
  config.wallHeightMeters = 2.50F;
  config.wallThicknessMeters = 1.0F;
  config.centerOnOrigin = true;
  return config;
}

iggy3d::EditableRoomDocument editableDocumentFromAscii() {
  const iggy3d::AsciiRoomSource source = iggy3d::parseAsciiRoomSource(
      "###\n"
      "#P#\n"
      "###\n",
      "tests/product_room_authoring_controller.iggyroom.txt");
  const iggy3d::AsciiRoomGridBuildResult grid = iggy3d::buildAsciiRoomGrid(source);
  const iggy3d::AsciiRoomToEditableRoomResult editable =
      iggy3d::buildEditableRoomFromAsciiRoom(grid.grid, compileConfig());
  return editable.document;
}

iggy3d::EditableRoomFloor extraFloorPrimitive() {
  iggy3d::EditableRoomFloor floor;
  floor.id = "controller_floor_1";
  floor.centerMeters = {2.0F, -0.05F, 0.0F};
  floor.sizeMeters = {1.0F, 0.10F, 1.0F};
  floor.semantics = iggy3d::defaultFloorSemantics("debug_floor");
  return floor;
}

iggy3d::EditableRoomWall extraWallPrimitive() {
  iggy3d::EditableRoomWall wall;
  wall.id = "controller_wall_1";
  wall.startMeters = {2.0F, 0.0F, -0.5F};
  wall.endMeters = {3.0F, 0.0F, -0.5F};
  wall.bottomY = 0.0F;
  wall.heightMeters = 2.50F;
  wall.thicknessMeters = 1.0F;
  wall.semantics = iggy3d::defaultWallSemantics("debug_wall");
  return wall;
}

bool initialSnapshotIsProjectionReadyFromAsciiDocument() {
  const iggy3d::ProductRoomAuthoringController controller(
      editableDocumentFromAscii());
  const iggy3d::ProductRoomAuthoringSnapshot& snapshot = controller.snapshot();

  return expect(snapshot.ready, "initial snapshot ready") &&
         expect(snapshot.status == "product_room_authoring_ready",
                "initial snapshot status") &&
         expect(snapshot.reasonCode == "product_room_authoring_ready",
                "initial snapshot reason") &&
         expect(snapshot.document.id == "controller_ascii_room",
                "document id preserved") &&
         expect(snapshot.documentFloorCount == 1U, "document floor count") &&
         expect(snapshot.documentWallCount == 8U, "document wall count") &&
         expect(snapshot.room.id == "controller_ascii_room", "room id") &&
         expect(snapshot.roomStaticMeshCount == 9U, "room static mesh count") &&
         expect(snapshot.roomSpatialSurfaceCount == 17U,
                "room spatial surface count") &&
         expect(snapshot.roomProjection.loaded, "projection loaded") &&
         expect(snapshot.projectedMeshCount == 9U, "projected mesh count") &&
         expect(snapshot.projectedFloorMeshCount == 1U,
                "projected floor count") &&
         expect(snapshot.projectedWallMeshCount == 8U,
                "projected wall count") &&
         expect(snapshot.undoDepth == 0U, "initial undo depth") &&
         expect(snapshot.redoDepth == 0U, "initial redo depth");
}

bool inputSourcesShareRoomEditCommandPath() {
  iggy3d::ProductRoomAuthoringController controller(editableDocumentFromAscii());
  const iggy3d::ProductRoomAuthoringSnapshot initial = controller.snapshot();

  const iggy3d::ProductRoomAuthoringCommandResult addFloor =
      controller.submit(iggy3d::ProductRoomAuthoringInputSource::Ai,
                        iggy3d::addFloorCommand(extraFloorPrimitive()));
  const iggy3d::ProductRoomAuthoringCommandResult addWall =
      controller.submit(iggy3d::ProductRoomAuthoringInputSource::Hotkey,
                        iggy3d::addWallCommand(extraWallPrimitive()));
  const iggy3d::ProductRoomAuthoringCommandResult deleteFloor =
      controller.submit(iggy3d::ProductRoomAuthoringInputSource::Mouse,
                        iggy3d::deleteFloorCommand("controller_floor_1"));

  const iggy3d::ProductRoomAuthoringSnapshot& snapshot = controller.snapshot();

  return expect(std::string_view{iggy3d::productRoomAuthoringInputSourceName(
                    iggy3d::ProductRoomAuthoringInputSource::Ai)} == "ai",
                "ai source name") &&
         expect(std::string_view{iggy3d::productRoomAuthoringInputSourceName(
                    iggy3d::ProductRoomAuthoringInputSource::Hotkey)} == "hotkey",
                "hotkey source name") &&
         expect(std::string_view{iggy3d::productRoomAuthoringInputSourceName(
                    iggy3d::ProductRoomAuthoringInputSource::Mouse)} == "mouse",
                "mouse source name") &&
         expect(addFloor.accepted, "add floor accepted") &&
         expect(addFloor.inputSource == iggy3d::ProductRoomAuthoringInputSource::Ai,
                "add floor source") &&
         expect(addFloor.status == "product_room_authoring_edit_applied",
                "add floor status") &&
         expect(addFloor.snapshot.projectedFloorMeshCount ==
                    initial.projectedFloorMeshCount + 1U,
                "add floor projected count") &&
         expect(addWall.accepted, "add wall accepted") &&
         expect(addWall.inputSource ==
                    iggy3d::ProductRoomAuthoringInputSource::Hotkey,
                "add wall source") &&
         expect(addWall.snapshot.projectedWallMeshCount ==
                    initial.projectedWallMeshCount + 1U,
                "add wall projected count") &&
         expect(deleteFloor.accepted, "delete floor accepted") &&
         expect(deleteFloor.inputSource ==
                    iggy3d::ProductRoomAuthoringInputSource::Mouse,
                "delete floor source") &&
         expect(snapshot.documentFloorCount == initial.documentFloorCount,
                "final floor count") &&
         expect(snapshot.documentWallCount == initial.documentWallCount + 1U,
                "final wall count") &&
         expect(snapshot.projectedFloorMeshCount ==
                    initial.projectedFloorMeshCount,
                "final projected floor count") &&
         expect(snapshot.projectedWallMeshCount ==
                    initial.projectedWallMeshCount + 1U,
                "final projected wall count") &&
         expect(snapshot.undoDepth == 3U, "undo depth after edits") &&
         expect(snapshot.redoDepth == 0U, "redo depth after edits");
}

bool rejectedEditDoesNotMutateSnapshot() {
  iggy3d::ProductRoomAuthoringController controller(editableDocumentFromAscii());
  const iggy3d::ProductRoomAuthoringSnapshot before = controller.snapshot();
  iggy3d::EditableRoomFloor duplicate = extraFloorPrimitive();
  duplicate.id = "floor_r1_c1";

  const iggy3d::ProductRoomAuthoringCommandResult result =
      controller.submit(iggy3d::ProductRoomAuthoringInputSource::Script,
                        iggy3d::addFloorCommand(duplicate));
  const iggy3d::ProductRoomAuthoringSnapshot& after = controller.snapshot();

  return expect(!result.accepted, "duplicate rejected") &&
         expect(result.status == "product_room_authoring_edit_rejected",
                "duplicate result status") &&
         expect(result.reasonCode == "room_edit_duplicate_id",
                "duplicate reason") &&
         expect(result.edit.status == iggy3d::RoomEditStatus::DuplicateId,
                "duplicate edit status") &&
         expect(result.snapshot.projectedMeshCount == before.projectedMeshCount,
                "result snapshot unchanged") &&
         expect(after.projectedMeshCount == before.projectedMeshCount,
                "controller projection unchanged") &&
         expect(after.documentFloorCount == before.documentFloorCount,
                "controller floor count unchanged") &&
         expect(after.undoDepth == before.undoDepth,
                "controller undo depth unchanged");
}

bool undoRedoRebuildsProjection() {
  iggy3d::ProductRoomAuthoringController controller(editableDocumentFromAscii());
  const iggy3d::ProductRoomAuthoringSnapshot initial = controller.snapshot();

  const iggy3d::ProductRoomAuthoringCommandResult add =
      controller.submit(iggy3d::ProductRoomAuthoringInputSource::Script,
                        iggy3d::addFloorCommand(extraFloorPrimitive()));
  const iggy3d::ProductRoomAuthoringCommandResult undo =
      controller.undo(iggy3d::ProductRoomAuthoringInputSource::Hotkey);
  const iggy3d::ProductRoomAuthoringCommandResult redo =
      controller.redo(iggy3d::ProductRoomAuthoringInputSource::Hotkey);

  return expect(add.accepted, "add before undo accepted") &&
         expect(add.snapshot.projectedFloorMeshCount ==
                    initial.projectedFloorMeshCount + 1U,
                "add increases floor projection") &&
         expect(undo.accepted, "undo accepted") &&
         expect(undo.status == "product_room_authoring_undo_applied",
                "undo status") &&
         expect(undo.edit.status == iggy3d::RoomEditStatus::UndoApplied,
                "undo edit status") &&
         expect(undo.snapshot.projectedFloorMeshCount ==
                    initial.projectedFloorMeshCount,
                "undo restores floor projection") &&
         expect(undo.snapshot.undoDepth == 0U, "undo depth after undo") &&
         expect(undo.snapshot.redoDepth == 1U, "redo depth after undo") &&
         expect(redo.accepted, "redo accepted") &&
         expect(redo.status == "product_room_authoring_redo_applied",
                "redo status") &&
         expect(redo.edit.status == iggy3d::RoomEditStatus::RedoApplied,
                "redo edit status") &&
         expect(redo.snapshot.projectedFloorMeshCount ==
                    initial.projectedFloorMeshCount + 1U,
                "redo reapplies floor projection") &&
         expect(redo.snapshot.undoDepth == 1U, "undo depth after redo") &&
         expect(redo.snapshot.redoDepth == 0U, "redo depth after redo");
}

}  // namespace

int main() {
  bool ok = true;
  ok = initialSnapshotIsProjectionReadyFromAsciiDocument() && ok;
  ok = inputSourcesShareRoomEditCommandPath() && ok;
  ok = rejectedEditDoesNotMutateSnapshot() && ok;
  ok = undoRedoRebuildsProjection() && ok;
  return ok ? 0 : 1;
}
