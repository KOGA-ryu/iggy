#include "app/iggy3d/AsciiRoomGrid.hpp"

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
  };
  const Expected expected[] = {
      {'#', iggy3d::AsciiRoomCellKind::Wall, false, true, true, ""},
      {'.', iggy3d::AsciiRoomCellKind::Floor, true, false, false, ""},
      {' ', iggy3d::AsciiRoomCellKind::Floor, true, false, false, ""},
      {'+', iggy3d::AsciiRoomCellKind::Door, true, false, false, "door"},
      {'s', iggy3d::AsciiRoomCellKind::SecretDoor, true, false, false, "secret_door"},
      {'P', iggy3d::AsciiRoomCellKind::PlayerSpawn, true, false, false, "player_spawn"},
      {'N', iggy3d::AsciiRoomCellKind::NpcSpawn, true, false, false, "npc_spawn"},
      {'M', iggy3d::AsciiRoomCellKind::MonsterSpawn, true, false, false, "monster_spawn"},
      {'$', iggy3d::AsciiRoomCellKind::Treasure, true, false, false, "treasure"},
      {'K', iggy3d::AsciiRoomCellKind::Key, true, false, false, "key"},
      {'T', iggy3d::AsciiRoomCellKind::Trap, true, false, false, "trap"},
      {'E', iggy3d::AsciiRoomCellKind::Exit, true, false, false, "exit"},
      {'?', iggy3d::AsciiRoomCellKind::Inspect, true, false, false, "inspect"},
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
  }
  return ok;
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
  ok = missingPlayerSpawnFails() && ok;
  ok = multiplePlayerSpawnsFail() && ok;
  ok = exactlyOnePlayerSpawnSucceeds() && ok;
  ok = noWalkableCellsFail() && ok;
  ok = coordinateConversionCentersRoom() && ok;
  ok = cellAtUsesRowMajorOrder() && ok;
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
