#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "app/iggy3d/ascii_room/AsciiRoomSource.hpp"

namespace iggy3d {

enum class AsciiRoomCellKind {
  Floor,
  Wall,
  Door,
  SecretDoor,
  PlayerSpawn,
  NpcSpawn,
  MonsterSpawn,
  Treasure,
  Key,
  Trap,
  Exit,
  Inspect,
};

enum class AsciiRoomTerrainKind : std::uint8_t {
  Flat,
  RampNorth,
  RampSouth,
  RampWest,
  RampEast,
  BlockedSteepEast,
};

struct AsciiRoomGlyphInfo {
  char glyph = '\0';
  AsciiRoomCellKind kind = AsciiRoomCellKind::Floor;
  bool walkable = true;
  bool blocksActor = false;
  bool blocksProjectile = false;
  std::string_view markerTag;
  AsciiRoomTerrainKind terrainKind = AsciiRoomTerrainKind::Flat;
  float elevationMeters = 0.0F;
  float riseMeters = 0.0F;
  std::string_view objectAssetId;
};

struct AsciiRoomCell {
  std::size_t row = 0;
  std::size_t column = 0;
  char glyph = '\0';
  AsciiRoomCellKind kind = AsciiRoomCellKind::Floor;
  bool walkable = false;
  bool blocksActor = false;
  bool blocksProjectile = false;
  std::string markerTag;
  std::string objectAssetId;
  AsciiRoomTerrainKind terrainKind = AsciiRoomTerrainKind::Flat;
  float elevationMeters = 0.0F;
  float riseMeters = 0.0F;
  std::size_t sourceOffset = 0;
};

struct AsciiRoomGrid {
  std::size_t width = 0;
  std::size_t height = 0;
  std::vector<AsciiRoomCell> cells;
  std::size_t playerSpawnCount = 0;
  std::size_t markerCount = 0;
  std::size_t objectCount = 0;
  std::size_t floorCount = 0;
  std::size_t wallCount = 0;
  std::size_t elevatedFloorCount = 0;
  std::size_t rampCount = 0;
  std::size_t blockedSlopeCount = 0;
};

struct AsciiRoomGridBuildResult {
  bool ok = false;
  std::string status = "ascii_room_ok";
  std::string reasonCode = "ascii_room_ok";
  AsciiRoomGrid grid;
  std::vector<AsciiRoomDiagnostic> diagnostics;
};

struct AsciiRoomWorldPosition {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
};

std::string_view asciiRoomCellKindName(AsciiRoomCellKind kind);
std::string_view asciiRoomTerrainKindName(AsciiRoomTerrainKind kind);
bool asciiRoomTerrainIsRamp(AsciiRoomTerrainKind kind);
std::optional<AsciiRoomGlyphInfo> asciiRoomGlyphInfo(char glyph);
AsciiRoomGridBuildResult buildAsciiRoomGrid(const AsciiRoomSource& source);
const AsciiRoomCell* asciiRoomCellAt(const AsciiRoomGrid& grid,
                                     std::size_t row,
                                     std::size_t column);
AsciiRoomWorldPosition asciiRoomCellCenter(std::size_t row,
                                           std::size_t column,
                                           std::size_t width,
                                           std::size_t height,
                                           double tileSize = 1.0,
                                           double elevation = 0.0);

}  // namespace iggy3d
