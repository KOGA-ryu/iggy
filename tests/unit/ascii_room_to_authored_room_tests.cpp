#include "app/iggy3d/AsciiRoomToAuthoredRoom.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(double lhs, double rhs) {
  return std::abs(lhs - rhs) < 0.0001;
}

iggy3d::AsciiRoomGrid canonicalGrid() {
  const iggy3d::AsciiRoomSource source = iggy3d::parseAsciiRoomSource(
      "#######\n#P..N.#\n#.+.$.#\n#..E..#\n#######\n",
      "canonical.iggyroom.txt");
  const iggy3d::AsciiRoomGridBuildResult grid = iggy3d::buildAsciiRoomGrid(source);
  return grid.grid;
}

iggy3d::AsciiRoomAuthoredRoomResult compileCanonical(
    iggy3d::AsciiRoomCompileConfig config = {}) {
  if (config.sourceName.empty()) {
    config.sourceName = "canonical.iggyroom.txt";
  }
  return iggy3d::compileAsciiRoomToAuthoredRoom(canonicalGrid(), config);
}

const iggy3d::AsciiRoomMarker* findMarker(
    const iggy3d::AsciiRoomAuthoredRoomResult& result,
    std::string_view tag) {
  for (const auto& marker : result.markers) {
    if (marker.tag == tag) {
      return &marker;
    }
  }
  return nullptr;
}

const iggy3d::AsciiRoomTerrainSurface* findTerrainSurface(
    const iggy3d::AsciiRoomAuthoredRoomResult& result,
    std::string_view floorId) {
  for (const auto& terrain : result.terrainSurfaces) {
    if (terrain.floorId == floorId) {
      return &terrain;
    }
  }
  return nullptr;
}

bool canonicalMapCompilesToExpectedCountsAndSourceFields() {
  const auto result = compileCanonical();
  return expect(result.ok, "canonical compile ok") &&
         expect(result.status == "ascii_room_ok", "canonical status") &&
         expect(result.authoredRoom.present, "authored room present") &&
         expect(result.authoredRoom.id == "ascii_room", "room id") &&
         expect(result.authoredRoom.version == 1U, "room version") &&
         expect(result.authoredRoom.source == "iggy3d.ascii_room", "room source") &&
         expect(result.authoredRoom.sourceFile == "canonical.iggyroom.txt",
                "room source file") &&
         expect(result.authoredRoom.sourceSubset == "ascii_room_authoring",
                "room source subset") &&
         expect(result.authoredRoom.floors.size() == 15U, "floor count") &&
         expect(result.authoredRoom.walls.size() == 20U, "wall count") &&
         expect(result.markers.size() == 5U, "marker count") &&
         expect(result.floorCount == 15U, "result floor count") &&
         expect(result.wallCount == 20U, "result wall count") &&
         expect(result.markerCount == 5U, "result marker count");
}

bool floorRecordMatchesDefaultConfig() {
  const auto result = compileCanonical();
  const auto& floor = result.authoredRoom.floors.front();
  return expect(floor.id == "floor_r1_c1", "floor id") &&
         expect(floor.storyIndex == 0, "floor story") &&
         expect(floor.centerMeters.x == -2.0F, "floor center x") &&
         expect(floor.centerMeters.y == -0.05F, "floor center y") &&
         expect(floor.centerMeters.z == -1.0F, "floor center z") &&
         expect(floor.sizeMeters.x == 1.0F, "floor size x") &&
         expect(floor.sizeMeters.y == 0.10F, "floor size y") &&
         expect(floor.sizeMeters.z == 1.0F, "floor size z") &&
         expect(floor.semantics.materialId == "debug_floor", "floor material") &&
         expect(floor.semantics.traversalTags.size() == 1U, "floor traversal tags") &&
         expect(floor.semantics.traversalTags[0] == "walkable", "floor traversal") &&
         expect(floor.semantics.gameplayTags.size() == 1U, "floor gameplay tags") &&
         expect(floor.semantics.gameplayTags[0] == "floor", "floor gameplay") &&
         expect(floor.semantics.walkable, "floor walkable") &&
         expect(!floor.semantics.blocksActor, "floor actor pass") &&
         expect(!floor.semantics.blocksProjectile, "floor projectile pass") &&
         expect(!floor.locked, "floor unlocked") &&
         expect(!floor.hidden, "floor visible");
}

bool wallRecordMatchesDefaultConfig() {
  const auto result = compileCanonical();
  const auto& wall = result.authoredRoom.walls.front();
  return expect(wall.id == "wall_r0_c0", "wall id") &&
         expect(wall.storyIndex == 0, "wall story") &&
         expect(wall.startMeters.x == -3.5F, "wall start x") &&
         expect(wall.startMeters.y == 0.0F, "wall start y") &&
         expect(wall.startMeters.z == -2.0F, "wall start z") &&
         expect(wall.endMeters.x == -2.5F, "wall end x") &&
         expect(wall.endMeters.y == 0.0F, "wall end y") &&
         expect(wall.endMeters.z == -2.0F, "wall end z") &&
         expect(wall.bottomY == 0.0F, "wall bottom") &&
         expect(wall.heightMeters == 2.50F, "wall height") &&
         expect(wall.thicknessMeters == 1.0F, "wall thickness") &&
         expect(wall.semantics.materialId == "debug_wall", "wall material") &&
         expect(wall.semantics.traversalTags.size() == 1U, "wall traversal tags") &&
         expect(wall.semantics.traversalTags[0] == "clamber_candidate",
                "wall traversal") &&
         expect(wall.semantics.gameplayTags.size() == 1U, "wall gameplay tags") &&
         expect(wall.semantics.gameplayTags[0] == "wall", "wall gameplay") &&
         expect(!wall.semantics.walkable, "wall not walkable") &&
         expect(wall.semantics.blocksActor, "wall blocks actor") &&
         expect(wall.semantics.blocksProjectile, "wall blocks projectile") &&
         expect(!wall.locked, "wall unlocked") &&
         expect(!wall.hidden, "wall visible");
}

bool customConfigChangesGeneratedDimensionsAndPositions() {
  iggy3d::AsciiRoomCompileConfig config;
  config.tileSizeMeters = 2.0F;
  config.floorThicknessMeters = 0.20F;
  config.wallHeightMeters = 3.0F;
  config.wallThicknessMeters = 0.50F;
  config.storyIndex = 2;
  config.roomId = "custom_room";
  config.sourceName = "custom.iggyroom.txt";
  const auto result = compileCanonical(config);
  const auto& floor = result.authoredRoom.floors.front();
  const auto& wall = result.authoredRoom.walls.front();
  return expect(result.authoredRoom.id == "custom_room", "custom room id") &&
         expect(result.authoredRoom.sourceFile == "custom.iggyroom.txt",
                "custom source") &&
         expect(floor.storyIndex == 2, "custom floor story") &&
         expect(floor.centerMeters.x == -4.0F, "custom floor x") &&
         expect(floor.centerMeters.y == -0.10F, "custom floor y") &&
         expect(floor.centerMeters.z == -2.0F, "custom floor z") &&
         expect(floor.sizeMeters.x == 2.0F, "custom floor size x") &&
         expect(floor.sizeMeters.y == 0.20F, "custom floor size y") &&
         expect(wall.storyIndex == 2, "custom wall story") &&
         expect(wall.startMeters.x == -7.0F, "custom wall start x") &&
         expect(wall.endMeters.x == -5.0F, "custom wall end x") &&
         expect(wall.startMeters.z == -4.0F, "custom wall z") &&
         expect(wall.heightMeters == 3.0F, "custom wall height") &&
         expect(wall.thicknessMeters == 0.50F, "custom wall thickness");
}

bool markerRecordsAreDeterministicSidecars() {
  const auto result = compileCanonical();
  const iggy3d::AsciiRoomMarker* player = findMarker(result, "player_spawn");
  const iggy3d::AsciiRoomMarker* door = findMarker(result, "door");
  const iggy3d::AsciiRoomMarker* treasure = findMarker(result, "treasure");
  return expect(player != nullptr, "player marker exists") &&
         expect(player->id == "marker_player_spawn_r1_c1", "player marker id") &&
         expect(player->glyph == 'P', "player marker glyph") &&
         expect(player->row == 1U && player->column == 1U, "player marker row col") &&
         expect(player->sourceLine == 2U && player->sourceColumn == 2U,
                "player marker source proof") &&
         expect(near(player->worldPosition.x, -2.0) &&
                    near(player->worldPosition.y, 0.05) &&
                    near(player->worldPosition.z, -1.0),
                "player marker position") &&
         expect(door != nullptr, "door marker exists") &&
         expect(door->id == "marker_door_r2_c2", "door marker id") &&
         expect(near(door->worldPosition.x, -1.0) &&
                    near(door->worldPosition.z, 0.0),
                "door marker position") &&
         expect(treasure != nullptr, "treasure marker exists") &&
         expect(treasure->id == "marker_treasure_r2_c4", "treasure marker id") &&
         expect(result.authoredRoom.floors.size() == 15U, "markers sidecar floors only") &&
         expect(result.authoredRoom.walls.size() == 20U, "markers sidecar walls only");
}

bool doorAndSecretDoorGenerateFloorAndMarkerOnly() {
  const auto source = iggy3d::parseAsciiRoomSource("#####\n#P+s#\n#####\n");
  const auto grid = iggy3d::buildAsciiRoomGrid(source);
  const auto result = iggy3d::compileAsciiRoomToAuthoredRoom(grid.grid);
  return expect(result.ok, "door secret compile ok") &&
         expect(result.authoredRoom.floors.size() == 3U,
                "door secret floor count") &&
         expect(result.authoredRoom.walls.size() == 12U,
                "door secret wall count") &&
         expect(result.markers.size() == 3U, "door secret marker count") &&
         expect(findMarker(result, "door") != nullptr, "door marker") &&
         expect(findMarker(result, "secret_door") != nullptr, "secret door marker");
}

bool terrainGlyphsCompileToSurfaceFacts() {
  const auto source =
      iggy3d::parseAsciiRoomSource("######\n#P1>!#\n######\n");
  const auto grid = iggy3d::buildAsciiRoomGrid(source);
  const auto result = iggy3d::compileAsciiRoomToAuthoredRoom(grid.grid);
  const iggy3d::AsciiRoomTerrainSurface* elevated =
      findTerrainSurface(result, "floor_r1_c2");
  const iggy3d::AsciiRoomTerrainSurface* ramp =
      findTerrainSurface(result, "floor_r1_c3");
  const iggy3d::AsciiRoomTerrainSurface* blocked =
      findTerrainSurface(result, "floor_r1_c4");
  return expect(result.ok, "terrain compile ok") &&
         expect(result.authoredRoom.floors.size() == 4U,
                "terrain floor count") &&
         expect(result.terrainSurfaces.size() == 4U,
                "terrain surface sidecars") &&
         expect(result.elevatedFloorCount == 1U, "elevated count") &&
         expect(result.rampCount == 1U, "ramp count") &&
         expect(result.blockedSlopeCount == 1U, "blocked slope count") &&
         expect(result.authoredRoom.floors[1].id == "floor_r1_c2",
                "elevated floor id") &&
         expect(near(result.authoredRoom.floors[1].centerMeters.y, 0.45),
                "elevated floor center y") &&
         expect(elevated != nullptr, "elevated surface sidecar") &&
         expect(elevated->topFacePoints.size() == 4U,
                "elevated sidecar point count") &&
         expect(near(elevated->topFacePoints[0].y, 0.5),
                "elevated sidecar height") &&
         expect(ramp != nullptr, "ramp surface sidecar") &&
         expect(ramp->ramp, "ramp sidecar flag") &&
         expect(!ramp->blockedSlope, "ramp not blocked") &&
         expect(near(ramp->topFacePoints[0].y, 0.0),
                "ramp sidecar low point") &&
         expect(near(ramp->topFacePoints[1].y, 0.5),
                "ramp sidecar high point") &&
         expect(near(ramp->normal.x, -0.4472136) &&
                    near(ramp->normal.y, 0.8944272),
                "ramp sidecar normal") &&
         expect(blocked != nullptr, "blocked surface sidecar") &&
         expect(blocked->blockedSlope, "blocked sidecar flag") &&
         expect(near(blocked->topFacePoints[1].y, 1.0),
                "blocked sidecar high point") &&
         expect(near(blocked->normal.y, 0.7071068),
                "blocked sidecar normal");
}

bool invalidGridRejectsDeterministically() {
  const iggy3d::AsciiRoomGrid empty;
  const auto result = iggy3d::compileAsciiRoomToAuthoredRoom(empty);
  return expect(!result.ok, "empty grid rejected") &&
         expect(result.status == "ascii_room_empty", "empty grid status") &&
         expect(!result.authoredRoom.present, "empty no authored room") &&
         expect(result.authoredRoom.floors.empty(), "empty no floors") &&
         expect(result.authoredRoom.walls.empty(), "empty no walls") &&
         expect(result.markers.empty(), "empty no markers") &&
         expect(!result.diagnostics.empty(), "empty diagnostic");
}

}  // namespace

int main() {
  bool ok = true;
  ok = canonicalMapCompilesToExpectedCountsAndSourceFields() && ok;
  ok = floorRecordMatchesDefaultConfig() && ok;
  ok = wallRecordMatchesDefaultConfig() && ok;
  ok = customConfigChangesGeneratedDimensionsAndPositions() && ok;
  ok = markerRecordsAreDeterministicSidecars() && ok;
  ok = doorAndSecretDoorGenerateFloorAndMarkerOnly() && ok;
  ok = terrainGlyphsCompileToSurfaceFacts() && ok;
  ok = invalidGridRejectsDeterministically() && ok;
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
