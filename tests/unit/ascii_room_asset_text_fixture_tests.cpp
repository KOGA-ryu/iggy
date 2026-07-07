#include "app/iggy3d/ascii_room/AsciiRoomAssetText.hpp"
#include "app/iggy3d/ascii_room/AsciiRoomToRoomAsset.hpp"
#include "content/assets/RoomAsset.hpp"

#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace {

constexpr std::string_view kAsciiFixturePath =
    "fixtures/rooms/ascii/training_room.iggyroom.txt";
constexpr std::string_view kRoomAssetFixturePath =
    "fixtures/rooms/ascii/training_room.room.iggy3d.toml";

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

iggy3d::RoomAsset buildExpectedRoomAsset() {
  const std::string text = readTextFile(std::filesystem::path{kAsciiFixturePath});
  const iggy3d::AsciiRoomSource source =
      iggy3d::parseAsciiRoomSource(text, std::string(kAsciiFixturePath));
  const iggy3d::AsciiRoomGridBuildResult grid =
      iggy3d::buildAsciiRoomGrid(source);
  iggy3d::AsciiRoomCompileConfig compileConfig;
  compileConfig.roomId = "training_room_ascii";
  compileConfig.sourceName = std::string(kAsciiFixturePath);
  const iggy3d::AsciiRoomAuthoredRoomResult authored =
      iggy3d::compileAsciiRoomToAuthoredRoom(grid.grid, compileConfig);
  iggy3d::AsciiRoomToRoomAssetConfig roomConfig;
  roomConfig.roomId = "training_room_ascii";
  roomConfig.sourceName = std::string(kAsciiFixturePath);
  return iggy3d::buildRoomAssetFromAsciiRoom(authored, roomConfig).room;
}

std::size_t countSurfacesWithRole(const iggy3d::RoomAsset& room,
                                  iggy3d::RoomSpatialSurfaceRole role) {
  std::size_t count = 0;
  for (const iggy3d::RoomSpatialSurface& surface : room.spatialSurfaces) {
    if (surface.role == role) {
      ++count;
    }
  }
  return count;
}

const iggy3d::RoomAnchorAsset* findAnchor(const iggy3d::RoomAsset& room,
                                          std::string_view id) {
  for (const iggy3d::RoomAnchorAsset& anchor : room.anchors) {
    if (anchor.id == id) {
      return &anchor;
    }
  }
  return nullptr;
}

bool anchorHasKind(const iggy3d::RoomAsset& room,
                   std::string_view id,
                   std::string_view kind) {
  const iggy3d::RoomAnchorAsset* anchor = findAnchor(room, id);
  return anchor != nullptr && anchor->kind == kind;
}

bool checkedInTextMatchesGeneratedExporterOutput() {
  const iggy3d::RoomAsset expectedRoom = buildExpectedRoomAsset();
  const iggy3d::AsciiRoomAssetTextResult generated =
      iggy3d::writeAsciiRoomAssetText(expectedRoom);
  const std::string checkedInText = readTextFile(std::filesystem::path{kRoomAssetFixturePath});
  const iggy3d::RoomAssetParseResult parsed =
      iggy3d::parseRoomAssetText(checkedInText);

  return expect(generated.ok, "generated text ok") &&
         expect(!checkedInText.empty(), "checked-in fixture not empty") &&
         expect(checkedInText == generated.text, "checked-in fixture byte parity") &&
         expect(parsed.ok, parsed.reason) &&
         expect(parsed.room.id == "training_room_ascii", "parsed room id") &&
         expect(parsed.room.source == "iggy3d.ascii_room", "parsed room source") &&
         expect(parsed.room.sourceFile == kAsciiFixturePath, "parsed source file") &&
         expect(parsed.room.sourceSubset == "ascii_room_authoring", "parsed source subset") &&
         expect(parsed.room.staticMeshes.size() == 36U, "parsed mesh count") &&
         expect(parsed.room.anchors.size() == 5U, "parsed anchor count") &&
         expect(parsed.room.spatialSurfaces.size() == 56U, "parsed surface count") &&
         expect(countSurfacesWithRole(parsed.room,
                                      iggy3d::RoomSpatialSurfaceRole::Walkable) == 15U,
                "walkable surface count") &&
         expect(countSurfacesWithRole(parsed.room,
                                      iggy3d::RoomSpatialSurfaceRole::Blocker) == 21U,
                "blocker surface count") &&
         expect(countSurfacesWithRole(
                    parsed.room,
                    iggy3d::RoomSpatialSurfaceRole::ProjectileBlocker) == 20U,
                "projectile blocker surface count") &&
         expect(anchorHasKind(parsed.room, "marker_player_spawn_r1_c1", "spawn"),
                "player spawn anchor") &&
         expect(anchorHasKind(parsed.room, "marker_npc_spawn_r1_c4", "npc"),
                "npc anchor") &&
         expect(anchorHasKind(parsed.room, "marker_treasure_r2_c4", "treasure"),
                "treasure anchor") &&
         expect(anchorHasKind(parsed.room, "marker_door_r2_c2", "door"),
                "door anchor") &&
         expect(anchorHasKind(parsed.room, "marker_exit_r3_c3", "exit"),
                "exit anchor");
}

// Regenerate the checked-in fixture from the current exporter output. Gated behind an env var so a
// deliberate exporter change (e.g. a new valid traversal tag) is a one-command update instead of a
// hand-edit. Run from the repo root: ASCII_ROOM_FIXTURE_REGEN=1 ./build/ascii_room_asset_text_fixture_tests
bool regenerateFixtureIfRequested() {
  if (std::getenv("ASCII_ROOM_FIXTURE_REGEN") == nullptr) {
    return false;
  }
  const iggy3d::RoomAsset expectedRoom = buildExpectedRoomAsset();
  const iggy3d::AsciiRoomAssetTextResult generated =
      iggy3d::writeAsciiRoomAssetText(expectedRoom);
  std::ofstream out(std::filesystem::path{kRoomAssetFixturePath}, std::ios::binary);
  if (!out) {
    std::cerr << "[regen] cannot write " << kRoomAssetFixturePath << " (run from the repo root)\n";
    return false;
  }
  out << generated.text;
  std::cout << "[regen] wrote fixture " << kRoomAssetFixturePath << '\n';
  return true;
}

}  // namespace

int main() {
  if (regenerateFixtureIfRequested()) {
    return EXIT_SUCCESS;
  }
  return checkedInTextMatchesGeneratedExporterOutput() ? EXIT_SUCCESS : EXIT_FAILURE;
}
