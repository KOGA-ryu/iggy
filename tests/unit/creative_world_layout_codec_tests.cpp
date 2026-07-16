#include "app/iggy3d/creative/world/WorldLayoutCodec.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

namespace cr = iggy3d::creative;

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeWorldLayout richLayout() {
  cr::CreativeWorldLayout layout;
  layout.stableKey = "estate layout\nlevel=0%";
  layout.terrainOwnership = cr::CreativeWorldLayoutTerrainOwnership::ReplaceAll;

  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "building 1";
  building.name = "Main House\nNorth Wing";
  building.rootMode = cr::CreativeBuildingRootMode::CreateRoom;
  building.rootFootprint = {{-4, -3}, {8, 7}};
  building.rootBaseLayer = -1;
  building.rootHeightCells = 4U;
  building.tags = {"interior", "author=map maker"};
  layout.buildings.push_back(building);

  cr::CreativeWorldLayoutRoom room;
  room.buildingIndex = 0U;
  room.stableKey = "room.study";
  room.name = "Study";
  room.footprint = {{0, 0}, {4, 3}};
  room.wallHeightCells = 4U;
  room.wallThicknessCells = 0.375;
  layout.rooms.push_back(room);

  cr::CreativeWorldLayoutBox floor;
  floor.buildingIndex = 0U;
  floor.kind = cr::CreativeObjectKind::Floor;
  floor.stableKey = "floor.main";
  floor.name = "Ground Floor";
  floor.footprint = {{-4, -3}, {8, 7}};
  layout.boxes.push_back(floor);

  cr::CreativeWorldLayoutWall wall;
  wall.buildingIndex = 0U;
  wall.stableKey = "wall.north";
  wall.name = "North Wall";
  wall.start = {-4, -3};
  wall.end = {8, -3};
  wall.heightCells = 4U;
  wall.thicknessCells = 0.375;
  layout.walls.push_back(wall);

  cr::CreativeWorldLayoutOpening opening;
  opening.wallIndex = 0U;
  opening.kind = cr::CreativeBuildingOpeningKind::Window;
  opening.stableKey = "window.north.1";
  opening.name = "Window = 1";
  opening.centerOffsetCells = 5.25;
  opening.widthCells = 1.5;
  opening.cutoutBottomCells = 1.0;
  opening.cutoutHeightCells = 1.25;
  opening.insertBottomCells = 1.0;
  opening.insertHeightCells = 1.25;
  opening.insertWidthCells = 1.5;
  opening.insertThicknessCells = 0.1;
  layout.openings.push_back(opening);

  cr::CreativeWorldLayoutOpening roomDoor;
  roomDoor.hostKind = cr::CreativeWorldLayoutOpeningHostKind::RoomEdge;
  roomDoor.roomIndex = 0U;
  roomDoor.roomEdge = cr::CreativeWorldLayoutRoomEdge::MaximumZ;
  roomDoor.kind = cr::CreativeBuildingOpeningKind::Door;
  roomDoor.stableKey = "door.study";
  roomDoor.name = "Study Door";
  roomDoor.centerOffsetCells = 2.0;
  layout.openings.push_back(roomDoor);

  cr::CreativeWorldLayoutTerrainProfile profile;
  profile.stableKey = "hill.west";
  profile.kind = cr::CreativeTerrainRecipeKind::Hill;
  profile.center = {-10, 2};
  profile.baseHeightCells = 2U;
  profile.radiusCells = 8U;
  profile.amplitudeCells = 5U;
  profile.spacingCells = 2U;
  profile.frequency = 2U;
  layout.terrainProfiles.push_back(profile);

  cr::CreativeWorldLayoutTerrainPath path;
  path.stableKey = "road.entry";
  path.kind = cr::CreativeTerrainRecipeKind::Road;
  path.firstPointIndex = 0U;
  path.pointCount = 3U;
  path.elevation = cr::CreativeTerrainPathElevation::Level;
  path.halfWidthCells = 2U;
  path.amplitudeCells = 1U;
  path.paintSurface = true;
  path.material = cr::CreativeTerrainMaterial::Dirt;
  layout.terrainPaths.push_back(path);
  layout.terrainPathPoints = {{{-6, 8}, 2U}, {{0, 8}, 2U}, {{0, 4}, 2U}};
  return layout;
}

bool deterministicRoundTripPreservesEveryTable() {
  const cr::CreativeWorldLayout source = richLayout();
  const cr::CreativeWorldLayoutEncodeResult first =
      cr::encodeCreativeWorldLayout(source);
  const cr::CreativeWorldLayoutEncodeResult second =
      cr::encodeCreativeWorldLayout(source);
  const cr::CreativeWorldLayoutDecodeResult decoded =
      cr::decodeCreativeWorldLayout(first.encodedText);
  const cr::CreativeWorldLayoutEncodeResult reencoded =
      cr::encodeCreativeWorldLayout(decoded.layout);

  return expect(first.accepted && second.accepted && decoded.accepted &&
                    reencoded.accepted,
                "rich layout codec operations accepted") &&
         expect(first.encodedText == second.encodedText &&
                    first.encodedText == reencoded.encodedText,
                "layout codec is byte deterministic") &&
         expect(decoded.layout.stableKey == source.stableKey,
                "special layout key round trips") &&
         expect(
             decoded.layout.buildings.size() == 1U &&
                 decoded.layout.buildings[0].name == source.buildings[0].name &&
                 decoded.layout.buildings[0].tags == source.buildings[0].tags,
             "building strings and tags round trip") &&
         expect(decoded.layout.rooms.size() == 1U &&
                    decoded.layout.rooms[0].footprint.maximum ==
                        source.rooms[0].footprint.maximum &&
                    decoded.layout.boxes.size() == 1U &&
                    decoded.layout.walls.size() == 1U &&
                    decoded.layout.openings.size() == 2U &&
                    decoded.layout.openings[1].hostKind ==
                        cr::CreativeWorldLayoutOpeningHostKind::RoomEdge,
                "building, room, and opening host tables round trip") &&
         expect(
             decoded.layout.terrainProfiles.size() == 1U &&
                 decoded.layout.terrainPaths.size() == 1U &&
                 decoded.layout.terrainPathPoints == source.terrainPathPoints,
             "terrain symbol tables round trip");
}

bool malformedAndNonFiniteInputsFailClosed() {
  cr::CreativeWorldLayout wrongEncodeSchema = richLayout();
  wrongEncodeSchema.schemaVersion = 99U;
  const cr::CreativeWorldLayoutEncodeResult invalidSchema =
      cr::encodeCreativeWorldLayout(wrongEncodeSchema);

  cr::CreativeWorldLayout layout = richLayout();
  layout.walls[0].thicknessCells = std::numeric_limits<double>::quiet_NaN();
  const cr::CreativeWorldLayoutEncodeResult nonFinite =
      cr::encodeCreativeWorldLayout(layout);

  const cr::CreativeWorldLayoutEncodeResult valid =
      cr::encodeCreativeWorldLayout(richLayout());
  const cr::CreativeWorldLayoutDecodeResult truncated =
      cr::decodeCreativeWorldLayout(
          valid.encodedText.substr(0U, valid.encodedText.find("END")));
  std::string unsupported = valid.encodedText;
  const std::string currentHeader =
      "IGGY3D_WORLD_LAYOUT " +
      std::to_string(cr::kCreativeWorldLayoutCodecVersion);
  unsupported.replace(unsupported.find(currentHeader), currentHeader.size(),
                      "IGGY3D_WORLD_LAYOUT 99");
  const cr::CreativeWorldLayoutDecodeResult wrongVersion =
      cr::decodeCreativeWorldLayout(unsupported);
  std::string wrongSchema = valid.encodedText;
  const std::string currentLayoutPrefix =
      "L " + std::to_string(cr::kCreativeWorldLayoutSchemaVersion) + " ";
  wrongSchema.replace(wrongSchema.find(currentLayoutPrefix),
                      currentLayoutPrefix.size(), "L 99 ");
  const cr::CreativeWorldLayoutDecodeResult schemaMismatch =
      cr::decodeCreativeWorldLayout(wrongSchema);
  const std::string overflowingCount =
      "IGGY3D_WORLD_LAYOUT 1\n"
      "L 1 776f726c645f6c61796f7574 0 18446744073709551615 0 0 0 0 0 0\n"
      "END\n";
  const cr::CreativeWorldLayoutDecodeResult overflow =
      cr::decodeCreativeWorldLayout(overflowingCount);

  return expect(!invalidSchema.accepted &&
                    invalidSchema.status ==
                        cr::CreativeWorldLayoutCodecStatus::InvalidRecord,
                "obsolete in-memory schema is not encoded") &&
         expect(!nonFinite.accepted &&
                    nonFinite.status ==
                        cr::CreativeWorldLayoutCodecStatus::NonFiniteValue,
                "non-finite source is not encoded") &&
         expect(!truncated.accepted, "truncated source is rejected") &&
         expect(!wrongVersion.accepted &&
                    wrongVersion.status ==
                        cr::CreativeWorldLayoutCodecStatus::UnsupportedVersion,
                "unsupported codec version is rejected") &&
         expect(!schemaMismatch.accepted &&
                    schemaMismatch.status ==
                        cr::CreativeWorldLayoutCodecStatus::InvalidRecord,
                "codec and source schema mismatch is rejected") &&
         expect(!overflow.accepted &&
                    overflow.status ==
                        cr::CreativeWorldLayoutCodecStatus::CapacityExceeded,
                "overflowing declared record counts fail before allocation");
}

bool versionOneSourceMigratesToCurrentSchema() {
  const std::string versionOne =
      "IGGY3D_WORLD_LAYOUT 1\n"
      "L 1 6c65676163795f6c61796f7574 0 0 0 0 0 0 0 0\n"
      "END\n";
  const cr::CreativeWorldLayoutDecodeResult decoded =
      cr::decodeCreativeWorldLayout(versionOne);
  const cr::CreativeWorldLayoutEncodeResult encoded =
      cr::encodeCreativeWorldLayout(decoded.layout);
  return expect(decoded.accepted &&
                    decoded.layout.schemaVersion ==
                        cr::kCreativeWorldLayoutSchemaVersion &&
                    decoded.layout.rooms.empty(),
                "version-one source migrates without fabricated rooms") &&
         expect(encoded.accepted &&
                    encoded.encodedText.starts_with("IGGY3D_WORLD_LAYOUT 2\n"),
                "migrated source writes the current codec version");
}

}  // namespace

int main() {
  const bool ok = deterministicRoundTripPreservesEveryTable() &&
                  malformedAndNonFiniteInputsFailClosed() &&
                  versionOneSourceMigratesToCurrentSchema();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
