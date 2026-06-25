#include "app/iggy3d/AsciiRoomGrid.hpp"
#include "app/iggy3d/AsciiRoomSource.hpp"
#include "app/iggy3d/AsciiRoomToEditableRoom.hpp"
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
  ok = invalidGridForwardsAuthoredRoomFailure() && ok;
  return ok ? 0 : 1;
}
