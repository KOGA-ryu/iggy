#include "app/iggy3d/ProductAsciiRoomEditing.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

namespace {

constexpr std::string_view kTrainingRoom =
    "#######\n"
    "#P..N.#\n"
    "#.+.$.#\n"
    "#..E..#\n"
    "#######\n";

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::ProductAsciiRoomAuthoringRequest trainingRoomRequest() {
  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceText = std::string(kTrainingRoom);
  request.sourceName = "inline/editing_training_room.iggyroom.txt";
  request.roomId = "editing_training_room";
  request.emitAssetText = false;
  return request;
}

iggy3d::EditableRoomFloor extraFloorPrimitive() {
  iggy3d::EditableRoomFloor floor;
  floor.id = "editing_floor_1";
  floor.centerMeters = {4.0F, -0.05F, 0.0F};
  floor.sizeMeters = {1.0F, 0.10F, 1.0F};
  floor.semantics = iggy3d::defaultFloorSemantics("debug_floor");
  return floor;
}

iggy3d::EditableRoomWall extraWallPrimitive() {
  iggy3d::EditableRoomWall wall;
  wall.id = "editing_wall_1";
  wall.startMeters = {4.0F, 0.0F, -0.5F};
  wall.endMeters = {5.0F, 0.0F, -0.5F};
  wall.bottomY = 0.0F;
  wall.heightMeters = 2.50F;
  wall.thicknessMeters = 1.0F;
  wall.semantics = iggy3d::defaultWallSemantics("debug_wall");
  return wall;
}

bool buildsControllerReadyEditingSession() {
  const iggy3d::ProductAsciiRoomEditingResult result =
      iggy3d::buildProductAsciiRoomEditing(trainingRoomRequest());

  return expect(result.ok, "editing result ok") &&
         expect(result.status == "product_ascii_room_editing_ready",
                "editing status") &&
         expect(result.reasonCode == "product_ascii_room_editing_ready",
                "editing reason") &&
         expect(result.failedStage == "none", "no failed stage") &&
         expect(result.source.status == "ascii_room_ok", "source ok") &&
         expect(result.grid.ok, "grid ok") &&
         expect(result.editableRoom.ok, "editable room ok") &&
         expect(result.snapshot.ready, "controller snapshot ready") &&
         expect(result.width == 7U, "width") &&
         expect(result.height == 5U, "height") &&
         expect(result.floorCount == 15U, "source floor count") &&
         expect(result.wallCount == 20U, "source wall count") &&
         expect(result.markerCount == 5U, "marker count") &&
         expect(result.documentFloorCount == 15U, "document floor count") &&
         expect(result.documentWallCount == 20U, "document wall count") &&
         expect(result.projectedMeshCount == 35U, "projected mesh count") &&
         expect(result.projectedFloorMeshCount == 15U,
                "projected floor count") &&
         expect(result.projectedWallMeshCount == 20U,
                "projected wall count") &&
         expect(result.snapshot.room.id == "editing_training_room",
                "snapshot room id") &&
         expect(result.snapshot.roomProjection.loaded, "room projection loaded");
}

bool returnedControllerAppliesFloorWallEdits() {
  iggy3d::ProductAsciiRoomEditingResult result =
      iggy3d::buildProductAsciiRoomEditing(trainingRoomRequest());
  const iggy3d::ProductRoomAuthoringSnapshot initial = result.controller.snapshot();

  const iggy3d::ProductRoomAuthoringCommandResult addFloor =
      result.controller.submit(iggy3d::ProductRoomAuthoringInputSource::Ai,
                               iggy3d::addFloorCommand(extraFloorPrimitive()));
  const iggy3d::ProductRoomAuthoringCommandResult addWall =
      result.controller.submit(iggy3d::ProductRoomAuthoringInputSource::Mouse,
                               iggy3d::addWallCommand(extraWallPrimitive()));
  const iggy3d::ProductRoomAuthoringSnapshot edited = result.controller.snapshot();

  return expect(result.ok, "editing result ok before edits") &&
         expect(addFloor.accepted, "floor edit accepted") &&
         expect(addFloor.snapshot.projectedFloorMeshCount ==
                    initial.projectedFloorMeshCount + 1U,
                "floor edit projection count") &&
         expect(addWall.accepted, "wall edit accepted") &&
         expect(addWall.snapshot.projectedWallMeshCount ==
                    initial.projectedWallMeshCount + 1U,
                "wall edit projection count") &&
         expect(edited.documentFloorCount == initial.documentFloorCount + 1U,
                "edited document floor count") &&
         expect(edited.documentWallCount == initial.documentWallCount + 1U,
                "edited document wall count") &&
         expect(edited.projectedMeshCount == initial.projectedMeshCount + 2U,
                "edited projected mesh count") &&
         expect(edited.undoDepth == 2U, "edited undo depth");
}

bool invalidAsciiDoesNotCreateEditingController() {
  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceText =
      "...\n"
      "...\n";
  request.sourceName = "inline/editing_invalid.iggyroom.txt";
  request.roomId = "editing_invalid_room";
  request.emitAssetText = false;

  const iggy3d::ProductAsciiRoomEditingResult result =
      iggy3d::buildProductAsciiRoomEditing(request);

  return expect(!result.ok, "editing result rejected") &&
         expect(result.failedStage == "grid", "failed at grid") &&
         expect(result.status == "ascii_room_missing_player_spawn", "status") &&
         expect(result.reasonCode == "ascii_room_missing_player_spawn",
                "reason") &&
         expect(result.source.status == "ascii_room_ok", "source ok") &&
         expect(!result.grid.ok, "grid rejected") &&
         expect(!result.snapshot.ready, "snapshot not ready") &&
         expect(result.documentFloorCount == 0U, "no document floors") &&
         expect(result.projectedMeshCount == 0U, "no projected meshes") &&
         expect(!result.diagnostics.empty(), "diagnostic forwarded");
}

}  // namespace

int main() {
  const bool ok = buildsControllerReadyEditingSession() &&
                  returnedControllerAppliesFloorWallEdits() &&
                  invalidAsciiDoesNotCreateEditingController();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
