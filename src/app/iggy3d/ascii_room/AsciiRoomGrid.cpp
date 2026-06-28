#include "app/iggy3d/ascii_room/AsciiRoomGrid.hpp"

#include <array>
#include <utility>

namespace iggy3d {
namespace {

constexpr float kElevationStepMeters = 0.5F;

constexpr std::array<AsciiRoomGlyphInfo, 26> kGlyphs{{
    {'#', AsciiRoomCellKind::Wall, false, true, true, "",
     AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, ""},
    {'J', AsciiRoomCellKind::Wall, false, true, true, "",
     AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, "",
     {0.8F, 0.8F, 0.8F}, "wall_jump"},
    {'.', AsciiRoomCellKind::Floor, true, false, false, "",
     AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, ""},
    {' ', AsciiRoomCellKind::Floor, true, false, false, "",
     AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, ""},
    {'0', AsciiRoomCellKind::Floor, true, false, false, "",
     AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, ""},
    {'1', AsciiRoomCellKind::Floor, true, false, false, "",
     AsciiRoomTerrainKind::Flat, kElevationStepMeters, 0.0F, ""},
    {'2', AsciiRoomCellKind::Floor, true, false, false, "",
     AsciiRoomTerrainKind::Flat, kElevationStepMeters * 2.0F, 0.0F, ""},
    {'3', AsciiRoomCellKind::Floor, true, false, false, "",
     AsciiRoomTerrainKind::Flat, kElevationStepMeters * 3.0F, 0.0F, ""},
    {'^', AsciiRoomCellKind::Floor, true, false, false, "",
     AsciiRoomTerrainKind::RampNorth, kElevationStepMeters / 2.0F,
     kElevationStepMeters, ""},
    {'v', AsciiRoomCellKind::Floor, true, false, false, "",
     AsciiRoomTerrainKind::RampSouth, kElevationStepMeters / 2.0F,
     kElevationStepMeters, ""},
    {'<', AsciiRoomCellKind::Floor, true, false, false, "",
     AsciiRoomTerrainKind::RampWest, kElevationStepMeters / 2.0F,
     kElevationStepMeters, ""},
    {'>', AsciiRoomCellKind::Floor, true, false, false, "",
     AsciiRoomTerrainKind::RampEast, kElevationStepMeters / 2.0F,
     kElevationStepMeters, ""},
    {'!', AsciiRoomCellKind::Floor, true, false, false, "",
     AsciiRoomTerrainKind::BlockedSteepEast, kElevationStepMeters, 1.0F, ""},
    {'+', AsciiRoomCellKind::Door, true, false, false, "door",
     AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, ""},
    {'s', AsciiRoomCellKind::SecretDoor, true, false, false, "secret_door",
     AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, ""},
    {'P', AsciiRoomCellKind::PlayerSpawn, true, false, false, "player_spawn",
     AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, ""},
    {'N', AsciiRoomCellKind::NpcSpawn, true, false, false, "npc_spawn",
     AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, ""},
    {'M', AsciiRoomCellKind::MonsterSpawn, true, false, false, "monster_spawn",
     AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, ""},
    {'$', AsciiRoomCellKind::Treasure, true, false, false, "treasure",
     AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, ""},
    {'K', AsciiRoomCellKind::Key, true, false, false, "key",
     AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, ""},
    {'T', AsciiRoomCellKind::Trap, true, false, false, "trap",
     AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, ""},
    {'R', AsciiRoomCellKind::ResetZone, true, false, false, "reset_zone",
     AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, ""},
    {'C', AsciiRoomCellKind::Floor, true, false, false, "",
     AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, "wood_crate_proxy"},
    {'L', AsciiRoomCellKind::Floor, true, false, false, "",
     AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, "movement_clamber_ledge_proxy",
     {2.0F, 1.7F, 1.0F}},
    {'E', AsciiRoomCellKind::Exit, true, false, false, "exit",
     AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, ""},
    {'?', AsciiRoomCellKind::Inspect, true, false, false, "inspect",
     AsciiRoomTerrainKind::Flat, 0.0F, 0.0F, ""},
}};

AsciiRoomDiagnostic diagnostic(std::string reason,
                               std::string message,
                               std::size_t row = 0,
                               std::size_t column = 0,
                               char glyph = '\0') {
  AsciiRoomDiagnostic out;
  out.severity = "error";
  out.reasonCode = std::move(reason);
  out.row = row;
  out.column = column;
  out.glyph = glyph;
  out.message = std::move(message);
  return out;
}

AsciiRoomGridBuildResult rejected(const AsciiRoomSource& source,
                                  std::string reason,
                                  std::string message) {
  AsciiRoomGridBuildResult result;
  result.status = reason;
  result.reasonCode = reason;
  if (!source.diagnostics.empty()) {
    result.diagnostics = source.diagnostics;
  } else {
    result.diagnostics.push_back(diagnostic(reason, message));
  }
  return result;
}

}  // namespace

std::string_view asciiRoomCellKindName(AsciiRoomCellKind kind) {
  switch (kind) {
    case AsciiRoomCellKind::Floor: return "floor";
    case AsciiRoomCellKind::Wall: return "wall";
    case AsciiRoomCellKind::Door: return "door";
    case AsciiRoomCellKind::SecretDoor: return "secret_door";
    case AsciiRoomCellKind::PlayerSpawn: return "player_spawn";
    case AsciiRoomCellKind::NpcSpawn: return "npc_spawn";
    case AsciiRoomCellKind::MonsterSpawn: return "monster_spawn";
    case AsciiRoomCellKind::Treasure: return "treasure";
    case AsciiRoomCellKind::Key: return "key";
    case AsciiRoomCellKind::Trap: return "trap";
    case AsciiRoomCellKind::Exit: return "exit";
    case AsciiRoomCellKind::ResetZone: return "reset_zone";
    case AsciiRoomCellKind::Inspect: return "inspect";
  }
  return "floor";
}

std::string_view asciiRoomTerrainKindName(AsciiRoomTerrainKind kind) {
  switch (kind) {
    case AsciiRoomTerrainKind::Flat: return "flat";
    case AsciiRoomTerrainKind::RampNorth: return "ramp_north";
    case AsciiRoomTerrainKind::RampSouth: return "ramp_south";
    case AsciiRoomTerrainKind::RampWest: return "ramp_west";
    case AsciiRoomTerrainKind::RampEast: return "ramp_east";
    case AsciiRoomTerrainKind::BlockedSteepEast: return "blocked_steep_east";
  }
  return "flat";
}

bool asciiRoomTerrainIsRamp(AsciiRoomTerrainKind kind) {
  return kind == AsciiRoomTerrainKind::RampNorth ||
         kind == AsciiRoomTerrainKind::RampSouth ||
         kind == AsciiRoomTerrainKind::RampWest ||
         kind == AsciiRoomTerrainKind::RampEast;
}

std::optional<AsciiRoomGlyphInfo> asciiRoomGlyphInfo(char glyph) {
  for (const AsciiRoomGlyphInfo& info : kGlyphs) {
    if (info.glyph == glyph) {
      return info;
    }
  }
  return std::nullopt;
}

AsciiRoomGridBuildResult buildAsciiRoomGrid(const AsciiRoomSource& source) {
  if (source.status != "ascii_room_ok") {
    return rejected(source, source.reasonCode, "source parse failed");
  }

  AsciiRoomGridBuildResult result;
  result.grid.width = source.width;
  result.grid.height = source.height;
  // branch-gate: BG-1167
  result.grid.layerCount = source.hasLayerDirectives ? source.layers.size() : 1U;
  result.grid.layerFloorSpacingMeters = source.layerFloorSpacingMeters;
  result.grid.cells.reserve(source.width * source.height * result.grid.layerCount);

  bool hasWalkable = false;
  const auto appendCells = [&](std::size_t layerIndex,
                               std::int32_t storyIndex,
                               const std::vector<std::string>& rows) {
    for (std::size_t row = 0; row < rows.size(); ++row) {
      for (std::size_t column = 0; column < rows[row].size(); ++column) {
        const char glyph = rows[row][column];
        // branch-gate: BG-1167
        if (source.hasLayerDirectives && glyph == ' ') {
          continue;
        }
        const std::optional<AsciiRoomGlyphInfo> info = asciiRoomGlyphInfo(glyph);
        // branch-gate: BG-1167
        if (!info.has_value()) {
          result.status = "ascii_room_unknown_glyph";
          result.reasonCode = result.status;
          result.diagnostics.push_back(diagnostic(result.status,
                                                  "unknown ASCII room glyph",
                                                  row,
                                                  column,
                                                  glyph));
          return;
        }

        AsciiRoomCell cell;
        cell.layerIndex = layerIndex;
        cell.storyIndex = storyIndex;
        cell.row = row;
        cell.column = column;
        cell.glyph = glyph;
        cell.kind = info->kind;
        cell.walkable = info->walkable;
        cell.blocksActor = info->blocksActor;
        cell.blocksProjectile = info->blocksProjectile;
        cell.markerTag = std::string(info->markerTag);
        cell.objectAssetId = std::string(info->objectAssetId);
        cell.terrainKind = info->terrainKind;
        cell.elevationMeters = info->elevationMeters +
                               static_cast<float>(storyIndex) *
                                   source.layerFloorSpacingMeters;
        cell.riseMeters = info->riseMeters;
        cell.objectSizeMeters = info->objectSizeMeters;
        cell.traversalTag = std::string(info->traversalTag);
        cell.sourceOffset = asciiRoomSourceOffset(source, layerIndex, row, column);
        hasWalkable = hasWalkable || cell.walkable;
        if (cell.kind == AsciiRoomCellKind::PlayerSpawn) {
          ++result.grid.playerSpawnCount;
        }
        if (!cell.markerTag.empty()) {
          ++result.grid.markerCount;
        }
        // branch-gate: BG-1130
        if (!cell.objectAssetId.empty()) {
          ++result.grid.objectCount;
        }
        // branch-gate: BG-1167
        if (cell.kind == AsciiRoomCellKind::Wall) {
          ++result.grid.wallCount;
        // branch-gate: BG-1167
        } else if (cell.walkable) {
          ++result.grid.floorCount;
          // branch-gate: BG-1167
          if (cell.terrainKind == AsciiRoomTerrainKind::BlockedSteepEast) {
            ++result.grid.blockedSlopeCount;
          // branch-gate: BG-1167
          } else if (asciiRoomTerrainIsRamp(cell.terrainKind)) {
            ++result.grid.rampCount;
          // branch-gate: BG-1167
          } else if (cell.elevationMeters > 0.0F) {
            ++result.grid.elevatedFloorCount;
          }
        }
        result.grid.cells.push_back(std::move(cell));
      }
    }
  };

  // branch-gate: BG-1167
  if (source.hasLayerDirectives) {
    for (std::size_t layerIndex = 0; layerIndex < source.layers.size();
         ++layerIndex) {
      appendCells(layerIndex,
                  source.layers[layerIndex].storyIndex,
                  source.layers[layerIndex].rows);
      // branch-gate: BG-1167
      if (result.status != "ascii_room_ok") {
        return result;
      }
    }
  } else {
    appendCells(0U, 0, source.rows);
    // branch-gate: BG-1167
    if (result.status != "ascii_room_ok") {
      return result;
    }
  }

  if (!hasWalkable) {
    result.status = "ascii_room_no_floor";
    result.reasonCode = result.status;
    result.diagnostics.push_back(diagnostic(result.status, "room has no walkable floor cells"));
    return result;
  }
  if (result.grid.playerSpawnCount == 0) {
    result.status = "ascii_room_missing_player_spawn";
    result.reasonCode = result.status;
    result.diagnostics.push_back(diagnostic(result.status, "room requires one player spawn"));
    return result;
  }
  if (result.grid.playerSpawnCount > 1) {
    result.status = "ascii_room_multiple_player_spawns";
    result.reasonCode = result.status;
    result.diagnostics.push_back(diagnostic(result.status, "room has multiple player spawns"));
    return result;
  }

  result.ok = true;
  result.status = "ascii_room_ok";
  result.reasonCode = "ascii_room_ok";
  return result;
}

const AsciiRoomCell* asciiRoomCellAt(const AsciiRoomGrid& grid,
                                     std::size_t row,
                                     std::size_t column) {
  return asciiRoomCellAt(grid, 0U, row, column);
}

const AsciiRoomCell* asciiRoomCellAt(const AsciiRoomGrid& grid,
                                     std::size_t layerIndex,
                                     std::size_t row,
                                     std::size_t column) {
  for (const AsciiRoomCell& cell : grid.cells) {
    if (cell.layerIndex == layerIndex && cell.row == row &&
        cell.column == column) {
      return &cell;
    }
  }
  return nullptr;
}

AsciiRoomWorldPosition asciiRoomCellCenter(std::size_t row,
                                           std::size_t column,
                                           std::size_t width,
                                           std::size_t height,
                                           double tileSize,
                                           double elevation) {
  AsciiRoomWorldPosition out;
  out.x = (static_cast<double>(column) -
           (static_cast<double>(width) - 1.0) / 2.0) * tileSize;
  out.y = elevation;
  out.z = (static_cast<double>(row) -
           (static_cast<double>(height) - 1.0) / 2.0) * tileSize;
  return out;
}

}  // namespace iggy3d
