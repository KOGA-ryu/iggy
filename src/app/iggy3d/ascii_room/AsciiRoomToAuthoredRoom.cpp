#include "app/iggy3d/ascii_room/AsciiRoomToAuthoredRoom.hpp"

#include <cstdint>
#include <cmath>
#include <string>
#include <string_view>

namespace iggy3d {
namespace {

AsciiRoomDiagnostic diagnostic(std::string reason, std::string message) {
  AsciiRoomDiagnostic out;
  out.severity = "error";
  out.reasonCode = std::move(reason);
  out.message = std::move(message);
  return out;
}

AsciiRoomWorldPosition cellPosition(const AsciiRoomCell& cell,
                                    const AsciiRoomGrid& grid,
                                    const AsciiRoomCompileConfig& config,
                                    float y) {
  if (config.centerOnOrigin) {
    return asciiRoomCellCenter(cell.row,
                               cell.column,
                               grid.width,
                               grid.height,
                               config.tileSizeMeters,
                               y);
  }
  return {static_cast<double>(cell.column) * config.tileSizeMeters,
          static_cast<double>(y),
          static_cast<double>(cell.row) * config.tileSizeMeters};
}

void appendUnique(std::vector<std::string>& tags, std::string value) {
  for (const std::string& tag : tags) {
    if (tag == value) {
      return;
    }
  }
  tags.push_back(std::move(value));
}

SaveAuthoredRoomSemanticsRecord floorSemantics(const AsciiRoomCell& cell) {
  SaveAuthoredRoomSemanticsRecord semantics;
  semantics.materialId = "debug_floor";
  semantics.traversalTags = {"walkable"};
  semantics.gameplayTags = {"floor"};
  if (cell.terrainKind == AsciiRoomTerrainKind::BlockedSteepEast) {
    appendUnique(semantics.traversalTags, "blocked_slope");
    appendUnique(semantics.gameplayTags, "blocked_slope");
  } else if (asciiRoomTerrainIsRamp(cell.terrainKind)) {
    appendUnique(semantics.traversalTags, "ramp");
    appendUnique(semantics.gameplayTags, "ramp");
  } else if (cell.elevationMeters > 0.0F) {
    appendUnique(semantics.traversalTags, "elevated_floor");
    appendUnique(semantics.gameplayTags, "elevated_floor");
  }
  if (cell.terrainKind != AsciiRoomTerrainKind::Flat ||
      cell.elevationMeters > 0.0F) {
    const std::string terrainTag =
        std::string{"terrain_"} + std::string(asciiRoomTerrainKindName(cell.terrainKind));
    appendUnique(semantics.traversalTags, terrainTag);
  }
  semantics.walkable = true;
  semantics.blocksActor = false;
  semantics.blocksProjectile = false;
  return semantics;
}

SaveAuthoredRoomSemanticsRecord wallSemantics() {
  SaveAuthoredRoomSemanticsRecord semantics;
  semantics.materialId = "debug_wall";
  semantics.traversalTags = {"clamber_candidate"};
  semantics.gameplayTags = {"wall"};
  semantics.walkable = false;
  semantics.blocksActor = true;
  semantics.blocksProjectile = true;
  return semantics;
}

SaveAuthoredRoomSemanticsRecord objectSemantics(std::string_view assetId) {
  SaveAuthoredRoomSemanticsRecord semantics;
  semantics.materialId = std::string(assetId);
  semantics.traversalTags = {"object", "prop", "crate"};
  semantics.gameplayTags = {"object", "prop", "crate"};
  semantics.walkable = false;
  semantics.blocksActor = true;
  semantics.blocksProjectile = true;
  return semantics;
}

std::string cellId(std::string_view prefix, const AsciiRoomCell& cell) {
  return std::string(prefix) + "_r" + std::to_string(cell.row) + "_c" +
         std::to_string(cell.column);
}

std::vector<Vec3> terrainTopFacePoints(const AsciiRoomCell& cell,
                                       const AsciiRoomGrid& grid,
                                       const AsciiRoomCompileConfig& config) {
  const AsciiRoomWorldPosition center = cellPosition(cell, grid, config, 0.0F);
  const float centerX = static_cast<float>(center.x);
  const float centerZ = static_cast<float>(center.z);
  const float halfTile = config.tileSizeMeters / 2.0F;
  const float lowY = cell.elevationMeters - cell.riseMeters / 2.0F;
  const float highY = cell.elevationMeters + cell.riseMeters / 2.0F;
  const float flatY = cell.elevationMeters;

  float northwestY = flatY;
  float northeastY = flatY;
  float southeastY = flatY;
  float southwestY = flatY;
  switch (cell.terrainKind) {
    case AsciiRoomTerrainKind::RampNorth:
      northwestY = highY;
      northeastY = highY;
      southeastY = lowY;
      southwestY = lowY;
      break;
    case AsciiRoomTerrainKind::RampSouth:
      northwestY = lowY;
      northeastY = lowY;
      southeastY = highY;
      southwestY = highY;
      break;
    case AsciiRoomTerrainKind::RampWest:
      northwestY = highY;
      northeastY = lowY;
      southeastY = lowY;
      southwestY = highY;
      break;
    case AsciiRoomTerrainKind::RampEast:
    case AsciiRoomTerrainKind::BlockedSteepEast:
      northwestY = lowY;
      northeastY = highY;
      southeastY = highY;
      southwestY = lowY;
      break;
    case AsciiRoomTerrainKind::Flat:
      break;
  }

  return {{centerX - halfTile, northwestY, centerZ - halfTile},
          {centerX + halfTile, northeastY, centerZ - halfTile},
          {centerX + halfTile, southeastY, centerZ + halfTile},
          {centerX - halfTile, southwestY, centerZ + halfTile}};
}

Vec3 terrainNormal(const AsciiRoomCell& cell, const AsciiRoomCompileConfig& config) {
  if (config.tileSizeMeters <= 0.0F || cell.riseMeters <= 0.0F) {
    return {0.0F, 1.0F, 0.0F};
  }

  const float grade = cell.riseMeters / config.tileSizeMeters;
  Vec3 normal{0.0F, 1.0F, 0.0F};
  switch (cell.terrainKind) {
    case AsciiRoomTerrainKind::RampNorth:
      normal = {0.0F, 1.0F, grade};
      break;
    case AsciiRoomTerrainKind::RampSouth:
      normal = {0.0F, 1.0F, -grade};
      break;
    case AsciiRoomTerrainKind::RampWest:
      normal = {grade, 1.0F, 0.0F};
      break;
    case AsciiRoomTerrainKind::RampEast:
    case AsciiRoomTerrainKind::BlockedSteepEast:
      normal = {-grade, 1.0F, 0.0F};
      break;
    case AsciiRoomTerrainKind::Flat:
      break;
  }

  const float length =
      std::sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
  if (!std::isfinite(length) || length <= 0.000001F) {
    return {0.0F, 1.0F, 0.0F};
  }
  return normal / length;
}

}  // namespace

AsciiRoomAuthoredRoomResult compileAsciiRoomToAuthoredRoom(
    const AsciiRoomGrid& grid,
    const AsciiRoomCompileConfig& config) {
  AsciiRoomAuthoredRoomResult result;
  if (grid.width == 0 || grid.height == 0 || grid.cells.empty()) {
    result.status = "ascii_room_empty";
    result.reasonCode = result.status;
    result.diagnostics.push_back(diagnostic(result.status, "ASCII room grid is empty"));
    return result;
  }

  result.authoredRoom.present = true;
  result.authoredRoom.id = config.roomId;
  result.authoredRoom.version = 1;
  result.authoredRoom.source = "iggy3d.ascii_room";
  result.authoredRoom.sourceFile = config.sourceName.empty() ? "ascii_room" : config.sourceName;
  result.authoredRoom.sourceSubset = "ascii_room_authoring";

  for (const AsciiRoomCell& cell : grid.cells) {
    const AsciiRoomWorldPosition center =
        cellPosition(cell, grid, config, cell.elevationMeters);
    if (cell.walkable) {
      SaveAuthoredRoomFloorRecord floor;
      floor.id = cellId("floor", cell);
      floor.storyIndex = config.storyIndex;
      floor.centerMeters = {static_cast<float>(center.x),
                            cell.elevationMeters - config.floorThicknessMeters / 2.0F,
                            static_cast<float>(center.z)};
      floor.sizeMeters = {config.tileSizeMeters,
                          config.floorThicknessMeters,
                          config.tileSizeMeters};
      floor.semantics = floorSemantics(cell);
      floor.locked = false;
      floor.hidden = false;
      const std::string floorId = floor.id;
      result.authoredRoom.floors.push_back(std::move(floor));

      AsciiRoomTerrainSurface terrain;
      terrain.floorId = floorId;
      terrain.kind = cell.terrainKind;
      terrain.topFacePoints = terrainTopFacePoints(cell, grid, config);
      terrain.normal = terrainNormal(cell, config);
      terrain.ramp = asciiRoomTerrainIsRamp(cell.terrainKind);
      terrain.blockedSlope = cell.terrainKind == AsciiRoomTerrainKind::BlockedSteepEast;
      result.terrainSurfaces.push_back(std::move(terrain));
    }

    if (cell.kind == AsciiRoomCellKind::Wall) {
      SaveAuthoredRoomWallRecord wall;
      wall.id = cellId("wall", cell);
      wall.storyIndex = config.storyIndex;
      wall.startMeters = {static_cast<float>(center.x) - config.tileSizeMeters / 2.0F,
                          0.0F,
                          static_cast<float>(center.z)};
      wall.endMeters = {static_cast<float>(center.x) + config.tileSizeMeters / 2.0F,
                        0.0F,
                        static_cast<float>(center.z)};
      wall.bottomY = 0.0F;
      wall.heightMeters = config.wallHeightMeters;
      wall.thicknessMeters = config.wallThicknessMeters;
      wall.semantics = wallSemantics();
      wall.locked = false;
      wall.hidden = false;
      result.authoredRoom.walls.push_back(std::move(wall));
    }

    // branch-gate: BG-1131
    if (!cell.objectAssetId.empty()) {
      SaveAuthoredRoomObjectRecord object;
      object.id = cellId("object_crate", cell);
      object.assetId = cell.objectAssetId;
      object.storyIndex = config.storyIndex;
      object.positionMeters = {static_cast<float>(center.x),
                               cell.elevationMeters + 0.4F,
                               static_cast<float>(center.z)};
      object.sizeMeters = {0.8F, 0.8F, 0.8F};
      object.yawDegrees = 0.0F;
      object.semantics = objectSemantics(object.assetId);
      object.locked = false;
      object.hidden = false;
      object.glyph = std::string(1, cell.glyph);
      object.row = static_cast<std::uint32_t>(cell.row);
      object.column = static_cast<std::uint32_t>(cell.column);
      object.sourceLine = static_cast<std::uint32_t>(cell.row + 1U);
      object.sourceColumn = static_cast<std::uint32_t>(cell.column + 1U);
      result.authoredRoom.objects.push_back(std::move(object));
    }

    if (!cell.markerTag.empty()) {
      AsciiRoomMarker marker;
      marker.id = "marker_" + cell.markerTag + "_r" + std::to_string(cell.row) +
                  "_c" + std::to_string(cell.column);
      marker.tag = cell.markerTag;
      marker.glyph = cell.glyph;
      marker.row = cell.row;
      marker.column = cell.column;
      marker.worldPosition =
          cellPosition(cell, grid, config, cell.elevationMeters + config.markerYMeters);
      marker.sourceLine = cell.row + 1U;
      marker.sourceColumn = cell.column + 1U;
      SaveAuthoredRoomMarkerRecord savedMarker;
      savedMarker.id = marker.id;
      savedMarker.tag = marker.tag;
      savedMarker.glyph = std::string(1, marker.glyph);
      savedMarker.row = static_cast<std::uint32_t>(marker.row);
      savedMarker.column = static_cast<std::uint32_t>(marker.column);
      savedMarker.positionMeters = {static_cast<float>(marker.worldPosition.x),
                                    static_cast<float>(marker.worldPosition.y),
                                    static_cast<float>(marker.worldPosition.z)};
      savedMarker.sourceLine = static_cast<std::uint32_t>(marker.sourceLine);
      savedMarker.sourceColumn = static_cast<std::uint32_t>(marker.sourceColumn);
      result.authoredRoom.markers.push_back(std::move(savedMarker));
      result.markers.push_back(std::move(marker));
    }
  }

  result.floorCount = result.authoredRoom.floors.size();
  result.wallCount = result.authoredRoom.walls.size();
  result.objectCount = result.authoredRoom.objects.size();
  result.markerCount = result.markers.size();
  result.elevatedFloorCount = grid.elevatedFloorCount;
  result.rampCount = grid.rampCount;
  result.blockedSlopeCount = grid.blockedSlopeCount;
  result.ok = true;
  result.status = "ascii_room_ok";
  result.reasonCode = "ascii_room_ok";
  return result;
}

}  // namespace iggy3d
