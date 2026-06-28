#include "app/iggy3d/ascii_room/AsciiRoomGrid.hpp"
#include "app/iggy3d/ascii_room/AsciiRoomSource.hpp"
#include "app/iggy3d/ascii_room/AsciiRoomToEditableRoom.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "runtime/session/SessionState.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

iggy3d::AsciiRoomGrid buildGrid(std::string_view text) {
  const iggy3d::AsciiRoomSource source =
      iggy3d::parseAsciiRoomSource(text, "tests/ascii_to_editable.iggyroom.txt");
  const iggy3d::AsciiRoomGridBuildResult grid = iggy3d::buildAsciiRoomGrid(source);
  return grid.grid;
}

iggy3d::AsciiRoomCompileConfig compileConfig() {
  iggy3d::AsciiRoomCompileConfig config;
  config.roomId = "editable_ascii_room";
  config.sourceName = "tests/ascii_to_editable.iggyroom.txt";
  config.tileSizeMeters = 1.0F;
  config.floorThicknessMeters = 0.10F;
  config.wallHeightMeters = 2.50F;
  config.wallThicknessMeters = 1.0F;
  config.centerOnOrigin = true;
  return config;
}

std::size_t countRoomMeshesWithRole(const iggy3d::RoomAsset& room,
                                    std::string_view role) {
  std::size_t count = 0;
  for (const iggy3d::RoomStaticMeshAsset& mesh : room.staticMeshes) {
    if (mesh.role == role) {
      ++count;
    }
  }
  return count;
}

std::size_t countProjectedMeshesWithRole(const iggy3d::SceneRoomProjection& room,
                                         std::string_view role) {
  std::size_t count = 0;
  for (const iggy3d::SceneRoomMeshItem& mesh : room.meshes) {
    if (mesh.role == role) {
      ++count;
    }
  }
  return count;
}

iggy3d::EditableRoomFloor extraFloorPrimitive() {
  iggy3d::EditableRoomFloor floor;
  floor.id = "edit_floor_1";
  floor.centerMeters = {2.0F, -0.05F, 0.0F};
  floor.sizeMeters = {1.0F, 0.10F, 1.0F};
  floor.semantics = iggy3d::defaultFloorSemantics("debug_floor");
  return floor;
}

iggy3d::EditableRoomWall extraWallPrimitive() {
  iggy3d::EditableRoomWall wall;
  wall.id = "edit_wall_1";
  wall.startMeters = {2.0F, 0.0F, -0.5F};
  wall.endMeters = {3.0F, 0.0F, -0.5F};
  wall.bottomY = 0.0F;
  wall.heightMeters = 2.50F;
  wall.thicknessMeters = 1.0F;
  wall.semantics = iggy3d::defaultWallSemantics("debug_wall");
  return wall;
}

bool asciiGridBuildsEditableFloorsAndWalls() {
  const iggy3d::AsciiRoomGrid grid = buildGrid(
      "###\n"
      "#P#\n"
      "###\n");
  const iggy3d::AsciiRoomToEditableRoomResult result =
      iggy3d::buildEditableRoomFromAsciiRoom(grid, compileConfig());

  const iggy3d::EditableRoomFloor* floor =
      iggy3d::findEditableFloor(result.document, "floor_r1_c1");
  const iggy3d::EditableRoomWall* wall =
      iggy3d::findEditableWall(result.document, "wall_r0_c0");

  return expect(result.ok, "editable result ok") &&
         expect(result.status == "ascii_room_editable_ready", "editable status") &&
         expect(result.document.id == "editable_ascii_room", "document id") &&
         expect(result.document.source == "iggy3d.ascii_room", "document source") &&
         expect(result.document.sourceFile == "tests/ascii_to_editable.iggyroom.txt",
                "document source file") &&
         expect(result.document.sourceSubset == "ascii_room_authoring",
                "document source subset") &&
         expect(result.floorCount == 1U, "result floor count") &&
         expect(result.wallCount == 8U, "result wall count") &&
         expect(result.markerCount == 1U, "result marker count preserved") &&
         expect(result.document.floors.size() == 1U, "document floor count") &&
         expect(result.document.walls.size() == 8U, "document wall count") &&
         expect(floor != nullptr, "floor exists") &&
         expect(floor != nullptr && floor->semantics.materialId == "debug_floor",
                "floor material") &&
         expect(floor != nullptr && floor->semantics.walkable, "floor walkable") &&
         expect(floor != nullptr && !floor->semantics.blocksActor,
                "floor does not block actor") &&
         expect(floor != nullptr && !floor->hidden && !floor->locked,
                "floor editable flags") &&
         expect(wall != nullptr, "wall exists") &&
         expect(wall != nullptr && wall->semantics.materialId == "debug_wall",
                "wall material") &&
         expect(wall != nullptr && wall->semantics.blocksActor,
                "wall blocks actor") &&
         expect(wall != nullptr && wall->semantics.blocksProjectile,
                "wall blocks projectile") &&
         expect(wall != nullptr && wall->semantics.traversalTags.size() == 1U &&
                    wall->semantics.traversalTags[0] == "clamber_candidate",
                "wall clamber candidate tag") &&
         expect(wall != nullptr && wall->heightMeters == 2.50F,
                "wall height") &&
         expect(wall != nullptr && wall->thicknessMeters == 1.0F,
                "wall thickness");
}

bool editableDocumentBakesBackToRoomAsset() {
  const iggy3d::AsciiRoomGrid grid = buildGrid(
      "###\n"
      "#P#\n"
      "###\n");
  const iggy3d::AsciiRoomToEditableRoomResult result =
      iggy3d::buildEditableRoomFromAsciiRoom(grid, compileConfig());
  const iggy3d::RoomBakeResult bake =
      iggy3d::bakeEditableRoomDocument(result.document);

  return expect(result.ok, "editable result ok before bake") &&
         expect(bake.ok, "bake ok") &&
         expect(std::string_view{bake.reasonCode} == "room_bake_ok",
                "bake status") &&
         expect(bake.room.id == "editable_ascii_room", "baked room id") &&
         expect(bake.room.source == "iggy3d.ascii_room", "baked source") &&
         expect(bake.room.sourceFile == "tests/ascii_to_editable.iggyroom.txt",
                "baked source file") &&
         expect(countRoomMeshesWithRole(bake.room, "floor") == result.floorCount,
                "baked floor mesh count") &&
         expect(countRoomMeshesWithRole(bake.room, "wall") == result.wallCount,
                "baked wall mesh count") &&
         expect(bake.room.spatialSurfaces.size() ==
                    result.floorCount + result.wallCount * 2U,
                "baked spatial surface count");
}

bool bakedEditableRoomProjectsFloorAndWallMeshes() {
  const iggy3d::AsciiRoomGrid grid = buildGrid(
      "###\n"
      "#P#\n"
      "###\n");
  const iggy3d::AsciiRoomToEditableRoomResult result =
      iggy3d::buildEditableRoomFromAsciiRoom(grid, compileConfig());
  const iggy3d::RoomBakeResult bake =
      iggy3d::bakeEditableRoomDocument(result.document);
  iggy3d::SessionState state;
  const iggy3d::SceneProjectionResult projection =
      iggy3d::buildSceneProjection(state, &bake.room);

  return expect(result.ok, "editable result ok before projection") &&
         expect(bake.ok, "bake ok before projection") &&
         expect(projection.room.loaded, "projected room loaded") &&
         expect(projection.room.assetId == "editable_ascii_room",
                "projection asset id") &&
         expect(projection.room.floorVisible, "projection floor visible") &&
         expect(projection.room.wallVisible, "projection wall visible") &&
         expect(projection.room.meshes.size() == result.floorCount + result.wallCount,
                "projected floor wall mesh count") &&
         expect(projection.room.staticMeshCount == bake.room.staticMeshes.size(),
                "projection source mesh count");
}

bool asciiCrateSurvivesEditableConversionAndBake() {
  const iggy3d::AsciiRoomGrid grid = buildGrid(
      "#####\n"
      "#PCE#\n"
      "#####\n");
  const iggy3d::AsciiRoomToEditableRoomResult result =
      iggy3d::buildEditableRoomFromAsciiRoom(grid, compileConfig());
  const iggy3d::EditableRoomObject* object =
      iggy3d::findEditableObject(result.document, "object_crate_r1_c2");
  const iggy3d::RoomBakeResult bake =
      iggy3d::bakeEditableRoomDocument(result.document);
  iggy3d::SessionState state;
  const iggy3d::SceneProjectionResult projection =
      iggy3d::buildSceneProjection(state, &bake.room);

  return expect(result.ok, "crate editable result ok") &&
         expect(result.objectCount == 1U, "crate editable object count") &&
         expect(result.document.objects.size() == 1U,
                "crate document object count") &&
         expect(object != nullptr, "crate editable object exists") &&
         expect(object != nullptr && object->assetId == "wood_crate_proxy",
                "crate editable asset") &&
         expect(object != nullptr && object->blocksActor,
                "crate editable blocks actor") &&
         expect(object != nullptr && object->blocksProjectile,
                "crate editable blocks projectile") &&
         expect(object != nullptr && object->positionMeters.y == 0.4F,
                "crate editable y") &&
         expect(object != nullptr && object->sizeMeters.x == 0.8F,
                "crate editable size") &&
         expect(bake.ok, "crate editable bake ok") &&
         expect(countRoomMeshesWithRole(bake.room, "prop") == 1U,
                "crate baked prop count") &&
         expect(bake.room.spatialSurfaces.size() ==
                    result.floorCount + result.wallCount * 2U + 2U,
                "crate baked blocker surfaces") &&
         expect(projection.room.loaded, "crate projection loaded") &&
         expect(projection.room.propVisible, "crate projection prop visible") &&
         expect(countProjectedMeshesWithRole(projection.room, "prop") == 1U,
                "crate projected prop count");
}

bool editCommandsChangeBakedProjectionCounts() {
  const iggy3d::AsciiRoomGrid grid = buildGrid(
      "###\n"
      "#P#\n"
      "###\n");
  const iggy3d::AsciiRoomToEditableRoomResult result =
      iggy3d::buildEditableRoomFromAsciiRoom(grid, compileConfig());
  iggy3d::EditableRoomSession session(result.document);

  const iggy3d::RoomBakeResult initialBake =
      iggy3d::bakeEditableRoomDocument(session.document());
  iggy3d::SessionState state;
  const iggy3d::SceneProjectionResult initialProjection =
      iggy3d::buildSceneProjection(state, &initialBake.room);

  const iggy3d::RoomEditResult addFloor =
      session.submit(iggy3d::addFloorCommand(extraFloorPrimitive()));
  const iggy3d::RoomEditResult addWall =
      session.submit(iggy3d::addWallCommand(extraWallPrimitive()));
  const iggy3d::RoomBakeResult addedBake =
      iggy3d::bakeEditableRoomDocument(session.document());
  const iggy3d::SceneProjectionResult addedProjection =
      iggy3d::buildSceneProjection(state, &addedBake.room);

  const iggy3d::RoomEditResult deleteFloor =
      session.submit(iggy3d::deleteFloorCommand("edit_floor_1"));
  const iggy3d::RoomEditResult deleteWall =
      session.submit(iggy3d::deleteWallCommand("edit_wall_1"));
  const iggy3d::RoomBakeResult deletedBake =
      iggy3d::bakeEditableRoomDocument(session.document());
  const iggy3d::SceneProjectionResult deletedProjection =
      iggy3d::buildSceneProjection(state, &deletedBake.room);

  return expect(result.ok, "editable result ok before edits") &&
         expect(initialBake.ok, "initial bake ok") &&
         expect(initialProjection.room.loaded, "initial projection loaded") &&
         expect(addFloor.status == iggy3d::RoomEditStatus::Applied,
                "add floor command applied") &&
         expect(addWall.status == iggy3d::RoomEditStatus::Applied,
                "add wall command applied") &&
         expect(addedBake.ok, "added bake ok") &&
         expect(addedProjection.room.loaded, "added projection loaded") &&
         expect(countRoomMeshesWithRole(addedBake.room, "floor") ==
                    countRoomMeshesWithRole(initialBake.room, "floor") + 1U,
                "baked floor count increased") &&
         expect(countRoomMeshesWithRole(addedBake.room, "wall") ==
                    countRoomMeshesWithRole(initialBake.room, "wall") + 1U,
                "baked wall count increased") &&
         expect(countProjectedMeshesWithRole(addedProjection.room, "floor") ==
                    countProjectedMeshesWithRole(initialProjection.room, "floor") + 1U,
                "projected floor count increased") &&
         expect(countProjectedMeshesWithRole(addedProjection.room, "wall") ==
                    countProjectedMeshesWithRole(initialProjection.room, "wall") + 1U,
                "projected wall count increased") &&
         expect(deleteFloor.status == iggy3d::RoomEditStatus::Applied,
                "delete floor command applied") &&
         expect(deleteFloor.affectedRuntimeIds.size() == 2U,
                "delete floor affected mesh and walkable") &&
         expect(deleteWall.status == iggy3d::RoomEditStatus::Applied,
                "delete wall command applied") &&
         expect(deleteWall.affectedRuntimeIds.size() == 3U,
                "delete wall affected mesh and blockers") &&
         expect(deletedBake.ok, "deleted bake ok") &&
         expect(deletedProjection.room.loaded, "deleted projection loaded") &&
         expect(countRoomMeshesWithRole(deletedBake.room, "floor") ==
                    countRoomMeshesWithRole(initialBake.room, "floor"),
                "baked floor count restored") &&
         expect(countRoomMeshesWithRole(deletedBake.room, "wall") ==
                    countRoomMeshesWithRole(initialBake.room, "wall"),
                "baked wall count restored") &&
         expect(countProjectedMeshesWithRole(deletedProjection.room, "floor") ==
                    countProjectedMeshesWithRole(initialProjection.room, "floor"),
                "projected floor count restored") &&
         expect(countProjectedMeshesWithRole(deletedProjection.room, "wall") ==
                    countProjectedMeshesWithRole(initialProjection.room, "wall"),
                "projected wall count restored");
}

bool invalidGridForwardsAuthoredRoomFailure() {
  const iggy3d::AsciiRoomGrid emptyGrid;
  const iggy3d::AsciiRoomToEditableRoomResult result =
      iggy3d::buildEditableRoomFromAsciiRoom(emptyGrid, compileConfig());

  return expect(!result.ok, "empty grid rejected") &&
         expect(result.status == "ascii_room_empty", "empty grid status") &&
         expect(result.reasonCode == "ascii_room_empty", "empty grid reason") &&
         expect(result.document.floors.empty(), "empty grid no floors") &&
         expect(result.document.walls.empty(), "empty grid no walls");
}

}  // namespace

int main() {
  bool ok = true;
  ok = asciiGridBuildsEditableFloorsAndWalls() && ok;
  ok = editableDocumentBakesBackToRoomAsset() && ok;
  ok = bakedEditableRoomProjectsFloorAndWallMeshes() && ok;
  ok = asciiCrateSurvivesEditableConversionAndBake() && ok;
  ok = editCommandsChangeBakedProjectionCounts() && ok;
  ok = invalidGridForwardsAuthoredRoomFailure() && ok;
  return ok ? 0 : 1;
}
