#include "app/iggy3d/ascii_room/AsciiRoomGrid.hpp"

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

std::size_t countKind(const iggy3d::AsciiRoomGrid& grid,
                      iggy3d::AsciiRoomCellKind kind) {
  std::size_t count = 0;
  for (const auto& cell : grid.cells) {
    if (cell.kind == kind) {
      ++count;
    }
  }
  return count;
}

bool canonicalMapFacts() {
  const iggy3d::AsciiRoomSource source = iggy3d::parseAsciiRoomSource(
      "#######\n#P..N.#\n#.+.$.#\n#..E..#\n#######\n");
  const iggy3d::AsciiRoomGridBuildResult result = iggy3d::buildAsciiRoomGrid(source);
  return expect(result.ok, "canonical ok") &&
         expect(result.grid.width == 7U, "canonical width") &&
         expect(result.grid.height == 5U, "canonical height") &&
         expect(result.grid.playerSpawnCount == 1U, "canonical player count") &&
         expect(countKind(result.grid, iggy3d::AsciiRoomCellKind::NpcSpawn) == 1U,
                "canonical npc count") &&
         expect(countKind(result.grid, iggy3d::AsciiRoomCellKind::Door) == 1U,
                "canonical door count") &&
         expect(countKind(result.grid, iggy3d::AsciiRoomCellKind::Treasure) == 1U,
                "canonical treasure count") &&
         expect(countKind(result.grid, iggy3d::AsciiRoomCellKind::Exit) == 1U,
                "canonical exit count");
}

bool glyphMappingsMatchContract() {
  struct Expected {
    char glyph;
    iggy3d::AsciiRoomCellKind kind;
    bool walkable;
    bool blocksActor;
    bool blocksProjectile;
    std::string_view markerTag;
    iggy3d::AsciiRoomTerrainKind terrainKind = iggy3d::AsciiRoomTerrainKind::Flat;
    float elevationMeters = 0.0F;
    float riseMeters = 0.0F;
    std::string_view objectAssetId;
    std::string_view traversalTag = "";
  };
  const Expected expected[] = {
      {'#', iggy3d::AsciiRoomCellKind::Wall, false, true, true, "",
       iggy3d::AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, ""},
      {'J', iggy3d::AsciiRoomCellKind::Wall, false, true, true, "",
       iggy3d::AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, "", "wall_jump"},
      {'.', iggy3d::AsciiRoomCellKind::Floor, true, false, false, "",
       iggy3d::AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, ""},
      {' ', iggy3d::AsciiRoomCellKind::Floor, true, false, false, "",
       iggy3d::AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, ""},
      {'0', iggy3d::AsciiRoomCellKind::Floor, true, false, false, "",
       iggy3d::AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, ""},
      {'1', iggy3d::AsciiRoomCellKind::Floor, true, false, false, "",
       iggy3d::AsciiRoomTerrainKind::Flat, 0.5F, 0.0F, ""},
      {'2', iggy3d::AsciiRoomCellKind::Floor, true, false, false, "",
       iggy3d::AsciiRoomTerrainKind::Flat, 1.0F, 0.0F, ""},
      {'3', iggy3d::AsciiRoomCellKind::Floor, true, false, false, "",
       iggy3d::AsciiRoomTerrainKind::Flat, 1.5F, 0.0F, ""},
      {'^', iggy3d::AsciiRoomCellKind::Floor, true, false, false, "",
       iggy3d::AsciiRoomTerrainKind::RampNorth, 0.25F, 0.5F, ""},
      {'v', iggy3d::AsciiRoomCellKind::Floor, true, false, false, "",
       iggy3d::AsciiRoomTerrainKind::RampSouth, 0.25F, 0.5F, ""},
      {'<', iggy3d::AsciiRoomCellKind::Floor, true, false, false, "",
       iggy3d::AsciiRoomTerrainKind::RampWest, 0.25F, 0.5F, ""},
      {'>', iggy3d::AsciiRoomCellKind::Floor, true, false, false, "",
       iggy3d::AsciiRoomTerrainKind::RampEast, 0.25F, 0.5F, ""},
      {'!', iggy3d::AsciiRoomCellKind::Floor, true, false, false, "",
       iggy3d::AsciiRoomTerrainKind::BlockedSteepEast, 0.5F, 1.0F, ""},
      {'+', iggy3d::AsciiRoomCellKind::Door, true, false, false, "door",
       iggy3d::AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, ""},
      {'s', iggy3d::AsciiRoomCellKind::SecretDoor, true, false, false, "secret_door",
       iggy3d::AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, ""},
      {'P', iggy3d::AsciiRoomCellKind::PlayerSpawn, true, false, false, "player_spawn",
       iggy3d::AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, ""},
      {'N', iggy3d::AsciiRoomCellKind::NpcSpawn, true, false, false, "npc_spawn",
       iggy3d::AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, ""},
      {'M', iggy3d::AsciiRoomCellKind::MonsterSpawn, true, false, false, "monster_spawn",
       iggy3d::AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, ""},
      {'$', iggy3d::AsciiRoomCellKind::Treasure, true, false, false, "treasure",
       iggy3d::AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, ""},
      {'K', iggy3d::AsciiRoomCellKind::Key, true, false, false, "key",
       iggy3d::AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, ""},
      {'T', iggy3d::AsciiRoomCellKind::Trap, true, false, false, "trap",
       iggy3d::AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, ""},
      {'C', iggy3d::AsciiRoomCellKind::Floor, true, false, false, "",
       iggy3d::AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, "wood_crate_proxy"},
      {'E', iggy3d::AsciiRoomCellKind::Exit, true, false, false, "exit",
       iggy3d::AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, ""},
      {'?', iggy3d::AsciiRoomCellKind::Inspect, true, false, false, "inspect",
       iggy3d::AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, ""},
  };

  bool ok = true;
  for (const Expected& item : expected) {
    const auto info = iggy3d::asciiRoomGlyphInfo(item.glyph);
    ok = expect(info.has_value(), "glyph exists") && ok;
    if (!info.has_value()) {
      continue;
    }
    ok = expect(info->kind == item.kind, "glyph kind") && ok;
    ok = expect(info->walkable == item.walkable, "glyph walkable") && ok;
    ok = expect(info->blocksActor == item.blocksActor, "glyph blocks actor") && ok;
    ok = expect(info->blocksProjectile == item.blocksProjectile,
                "glyph blocks projectile") && ok;
    ok = expect(info->markerTag == item.markerTag, "glyph marker tag") && ok;
    ok = expect(info->terrainKind == item.terrainKind, "glyph terrain kind") && ok;
    ok = expect(info->elevationMeters == item.elevationMeters,
                "glyph elevation") && ok;
    ok = expect(info->riseMeters == item.riseMeters, "glyph rise") && ok;
    ok = expect(info->objectAssetId == item.objectAssetId,
                "glyph object asset") && ok;
    ok = expect(info->traversalTag == item.traversalTag,
                "glyph traversal tag") && ok;
  }
  return ok;
}

bool crateGlyphCreatesWalkableObjectCell() {
  const auto source = iggy3d::parseAsciiRoomSource("#####\n#PCE#\n#####\n");
  const auto result = iggy3d::buildAsciiRoomGrid(source);
  const iggy3d::AsciiRoomCell* crate = iggy3d::asciiRoomCellAt(result.grid, 1, 2);
  return expect(result.ok, "crate glyph grid ok") &&
         expect(result.grid.floorCount == 3U, "crate floor count") &&
         expect(result.grid.objectCount == 1U, "crate object count") &&
         expect(crate != nullptr, "crate cell exists") &&
         expect(crate != nullptr && crate->glyph == 'C', "crate glyph") &&
         expect(crate != nullptr && crate->walkable, "crate walkable") &&
         expect(crate != nullptr && crate->kind == iggy3d::AsciiRoomCellKind::Floor,
                "crate floor kind") &&
         expect(crate != nullptr && crate->markerTag.empty(), "crate no marker") &&
         expect(crate != nullptr && crate->objectAssetId == "wood_crate_proxy",
                "crate object asset");
}

bool wallJumpGlyphCreatesAuthoredWallCell() {
  const auto source = iggy3d::parseAsciiRoomSource("#####\n#P.E#\n##J##\n");
  const auto result = iggy3d::buildAsciiRoomGrid(source);
  const iggy3d::AsciiRoomCell* wall = iggy3d::asciiRoomCellAt(result.grid, 2, 2);
  return expect(result.ok, "wall jump glyph grid ok") &&
         expect(result.grid.wallCount == 12U, "wall jump wall count") &&
         expect(wall != nullptr, "wall jump cell exists") &&
         expect(wall != nullptr && wall->glyph == 'J', "wall jump glyph") &&
         expect(wall != nullptr && !wall->walkable, "wall jump not walkable") &&
         expect(wall != nullptr && wall->kind == iggy3d::AsciiRoomCellKind::Wall,
                "wall jump wall kind") &&
         expect(wall != nullptr && wall->blocksActor, "wall jump blocks actor") &&
         expect(wall != nullptr && wall->blocksProjectile,
                "wall jump blocks projectile") &&
         expect(wall != nullptr && wall->traversalTag == "wall_jump",
                "wall jump traversal tag");
}

bool terrainGlyphFacts() {
  const auto source = iggy3d::parseAsciiRoomSource("########\n#P1>!^v#\n########\n");
  const auto result = iggy3d::buildAsciiRoomGrid(source);
  const iggy3d::AsciiRoomCell* elevated = iggy3d::asciiRoomCellAt(result.grid, 1, 2);
  const iggy3d::AsciiRoomCell* rampEast = iggy3d::asciiRoomCellAt(result.grid, 1, 3);
  const iggy3d::AsciiRoomCell* blocked = iggy3d::asciiRoomCellAt(result.grid, 1, 4);
  const iggy3d::AsciiRoomCell* rampNorth = iggy3d::asciiRoomCellAt(result.grid, 1, 5);
  const iggy3d::AsciiRoomCell* rampSouth = iggy3d::asciiRoomCellAt(result.grid, 1, 6);
  return expect(result.ok, "terrain glyph map ok") &&
         expect(result.grid.floorCount == 6U, "terrain floor count") &&
         expect(result.grid.elevatedFloorCount == 1U,
                "terrain elevated floor count") &&
         expect(result.grid.rampCount == 3U, "terrain ramp count") &&
         expect(result.grid.blockedSlopeCount == 1U,
                "terrain blocked slope count") &&
         expect(elevated != nullptr, "elevated cell exists") &&
         expect(elevated->terrainKind == iggy3d::AsciiRoomTerrainKind::Flat,
                "elevated terrain flat") &&
         expect(elevated->elevationMeters == 0.5F, "elevated height") &&
         expect(rampEast != nullptr, "ramp east cell exists") &&
         expect(rampEast->terrainKind == iggy3d::AsciiRoomTerrainKind::RampEast,
                "ramp east kind") &&
         expect(rampEast->riseMeters == 0.5F, "ramp east rise") &&
         expect(blocked != nullptr, "blocked cell exists") &&
         expect(blocked->terrainKind == iggy3d::AsciiRoomTerrainKind::BlockedSteepEast,
                "blocked kind") &&
         expect(blocked->riseMeters == 1.0F, "blocked rise") &&
         expect(rampNorth != nullptr && rampSouth != nullptr,
                "north south ramps exist") &&
         expect(iggy3d::asciiRoomTerrainKindName(rampNorth->terrainKind) ==
                    "ramp_north",
                "terrain name north") &&
         expect(iggy3d::asciiRoomTerrainIsRamp(rampSouth->terrainKind),
                "terrain ramp helper");
}

bool missingPlayerSpawnFails() {
  const auto source = iggy3d::parseAsciiRoomSource("###\n#.#\n###");
  const auto result = iggy3d::buildAsciiRoomGrid(source);
  return expect(!result.ok, "missing spawn rejected") &&
         expect(result.status == "ascii_room_missing_player_spawn",
                "missing spawn status");
}

bool multiplePlayerSpawnsFail() {
  const auto source = iggy3d::parseAsciiRoomSource("####\n#PP#\n####");
  const auto result = iggy3d::buildAsciiRoomGrid(source);
  return expect(!result.ok, "multiple spawn rejected") &&
         expect(result.status == "ascii_room_multiple_player_spawns",
                "multiple spawn status");
}

bool exactlyOnePlayerSpawnSucceeds() {
  const auto source = iggy3d::parseAsciiRoomSource("###\n#P#\n###");
  const auto result = iggy3d::buildAsciiRoomGrid(source);
  return expect(result.ok, "one spawn succeeds") &&
         expect(result.grid.playerSpawnCount == 1U, "one spawn count");
}

bool noWalkableCellsFail() {
  const auto source = iggy3d::parseAsciiRoomSource("###\n###\n###");
  const auto result = iggy3d::buildAsciiRoomGrid(source);
  return expect(!result.ok, "no floor rejected") &&
         expect(result.status == "ascii_room_no_floor", "no floor status");
}

bool coordinateConversionCentersRoom() {
  const auto center = iggy3d::asciiRoomCellCenter(2, 3, 7, 5);
  const auto northwest = iggy3d::asciiRoomCellCenter(0, 0, 7, 5);
  const auto southeast = iggy3d::asciiRoomCellCenter(4, 6, 7, 5);
  const auto elevated = iggy3d::asciiRoomCellCenter(1, 2, 5, 3, 2.0, 0.5);
  return expect(center.x == 0.0 && center.y == 0.0 && center.z == 0.0,
                "center origin") &&
         expect(northwest.x == -3.0 && northwest.z == -2.0,
                "northwest centered") &&
         expect(southeast.x == 3.0 && southeast.z == 2.0,
                "southeast centered") &&
         expect(elevated.x == 0.0 && elevated.y == 0.5 && elevated.z == 0.0,
                "tile size elevation");
}

bool cellAtUsesRowMajorOrder() {
  const auto source = iggy3d::parseAsciiRoomSource("###\n#P#\n###");
  const auto result = iggy3d::buildAsciiRoomGrid(source);
  const iggy3d::AsciiRoomCell* cell = iggy3d::asciiRoomCellAt(result.grid, 1, 1);
  return expect(cell != nullptr, "cell at exists") &&
         expect(cell->glyph == 'P', "cell at glyph") &&
         expect(cell->sourceOffset == 5U, "cell source offset");
}

}  // namespace

int main() {
  bool ok = true;
  ok = canonicalMapFacts() && ok;
  ok = glyphMappingsMatchContract() && ok;
  ok = crateGlyphCreatesWalkableObjectCell() && ok;
  ok = wallJumpGlyphCreatesAuthoredWallCell() && ok;
  ok = terrainGlyphFacts() && ok;
  ok = missingPlayerSpawnFails() && ok;
  ok = multiplePlayerSpawnsFail() && ok;
  ok = exactlyOnePlayerSpawnSucceeds() && ok;
  ok = noWalkableCellsFail() && ok;
  ok = coordinateConversionCentersRoom() && ok;
  ok = cellAtUsesRowMajorOrder() && ok;
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
