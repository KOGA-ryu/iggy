#include "app/iggy3d/AsciiRoomToAuthoredRoom.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace {

constexpr std::string_view kFixturePath =
    "fixtures/rooms/ascii/training_room.iggyroom.txt";

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

std::string readTextFile(const std::filesystem::path& path) {
  std::ifstream input(path);
  std::ostringstream out;
  out << input.rdbuf();
  return out.str();
}

std::size_t countKind(const iggy3d::AsciiRoomGrid& grid,
                      iggy3d::AsciiRoomCellKind kind) {
  std::size_t count = 0;
  for (const iggy3d::AsciiRoomCell& cell : grid.cells) {
    if (cell.kind == kind) {
      ++count;
    }
  }
  return count;
}

bool hasMarkerId(const iggy3d::AsciiRoomAuthoredRoomResult& result,
                 std::string_view id) {
  for (const iggy3d::AsciiRoomMarker& marker : result.markers) {
    if (marker.id == id) {
      return true;
    }
  }
  return false;
}

bool fixtureCompilesToDeterministicProof() {
  const std::filesystem::path path{kFixturePath};
  const bool exists = std::filesystem::exists(path);
  const std::string text = exists ? readTextFile(path) : std::string{};
  const iggy3d::AsciiRoomSource source =
      iggy3d::parseAsciiRoomSource(text, std::string(kFixturePath));
  const iggy3d::AsciiRoomGridBuildResult grid =
      iggy3d::buildAsciiRoomGrid(source);

  iggy3d::AsciiRoomCompileConfig config;
  config.roomId = "training_room_ascii";
  config.sourceName = std::string(kFixturePath);
  const iggy3d::AsciiRoomAuthoredRoomResult compiled =
      iggy3d::compileAsciiRoomToAuthoredRoom(grid.grid, config);

  return expect(exists, "fixture exists") &&
         expect(!text.empty(), "fixture not empty") &&
         expect(source.status == "ascii_room_ok", "source status") &&
         expect(source.width == 7U, "source width") &&
         expect(source.height == 5U, "source height") &&
         expect(grid.ok, "grid ok") &&
         expect(grid.grid.width == 7U, "grid width") &&
         expect(grid.grid.height == 5U, "grid height") &&
         expect(grid.grid.playerSpawnCount == 1U, "player spawn count") &&
         expect(grid.grid.floorCount == 15U, "grid floor count") &&
         expect(grid.grid.wallCount == 20U, "grid wall count") &&
         expect(grid.grid.markerCount == 5U, "grid marker count") &&
         expect(countKind(grid.grid, iggy3d::AsciiRoomCellKind::NpcSpawn) == 1U,
                "npc count") &&
         expect(countKind(grid.grid, iggy3d::AsciiRoomCellKind::Door) == 1U,
                "door count") &&
         expect(countKind(grid.grid, iggy3d::AsciiRoomCellKind::Treasure) == 1U,
                "treasure count") &&
         expect(countKind(grid.grid, iggy3d::AsciiRoomCellKind::Exit) == 1U,
                "exit count") &&
         expect(compiled.ok, "compile ok") &&
         expect(compiled.status == "ascii_room_ok", "compile status") &&
         expect(compiled.authoredRoom.present, "authored room present") &&
         expect(compiled.authoredRoom.id == "training_room_ascii",
                "authored room id") &&
         expect(compiled.authoredRoom.source == "iggy3d.ascii_room",
                "authored room source") &&
         expect(compiled.authoredRoom.sourceFile == kFixturePath,
                "authored room source file") &&
         expect(compiled.authoredRoom.sourceSubset == "ascii_room_authoring",
                "authored room source subset") &&
         expect(compiled.authoredRoom.floors.size() == 15U,
                "compiled floor count") &&
         expect(compiled.authoredRoom.walls.size() == 20U,
                "compiled wall count") &&
         expect(compiled.markers.size() == 5U, "compiled marker count") &&
         expect(hasMarkerId(compiled, "marker_player_spawn_r1_c1"),
                "player marker id") &&
         expect(hasMarkerId(compiled, "marker_npc_spawn_r1_c4"),
                "npc marker id") &&
         expect(hasMarkerId(compiled, "marker_door_r2_c2"),
                "door marker id") &&
         expect(hasMarkerId(compiled, "marker_treasure_r2_c4"),
                "treasure marker id") &&
         expect(hasMarkerId(compiled, "marker_exit_r3_c3"),
                "exit marker id");
}

}  // namespace

int main() {
  return fixtureCompilesToDeterministicProof() ? EXIT_SUCCESS : EXIT_FAILURE;
}
