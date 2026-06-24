#include "app/iggy3d/AsciiRoomToAuthoredRoom.hpp"

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

SaveAuthoredRoomSemanticsRecord floorSemantics() {
  SaveAuthoredRoomSemanticsRecord semantics;
  semantics.materialId = "debug_floor";
  semantics.traversalTags = {"walkable"};
  semantics.gameplayTags = {"floor"};
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

std::string cellId(std::string_view prefix, const AsciiRoomCell& cell) {
  return std::string(prefix) + "_r" + std::to_string(cell.row) + "_c" +
         std::to_string(cell.column);
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
    const AsciiRoomWorldPosition center = cellPosition(cell, grid, config, 0.0F);
    if (cell.walkable) {
      SaveAuthoredRoomFloorRecord floor;
      floor.id = cellId("floor", cell);
      floor.storyIndex = config.storyIndex;
      floor.centerMeters = {static_cast<float>(center.x),
                            -config.floorThicknessMeters / 2.0F,
                            static_cast<float>(center.z)};
      floor.sizeMeters = {config.tileSizeMeters,
                          config.floorThicknessMeters,
                          config.tileSizeMeters};
      floor.semantics = floorSemantics();
      floor.locked = false;
      floor.hidden = false;
      result.authoredRoom.floors.push_back(std::move(floor));
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

    if (!cell.markerTag.empty()) {
      AsciiRoomMarker marker;
      marker.id = "marker_" + cell.markerTag + "_r" + std::to_string(cell.row) +
                  "_c" + std::to_string(cell.column);
      marker.tag = cell.markerTag;
      marker.glyph = cell.glyph;
      marker.row = cell.row;
      marker.column = cell.column;
      marker.worldPosition = cellPosition(cell, grid, config, config.markerYMeters);
      marker.sourceLine = cell.row + 1U;
      marker.sourceColumn = cell.column + 1U;
      result.markers.push_back(std::move(marker));
    }
  }

  result.floorCount = result.authoredRoom.floors.size();
  result.wallCount = result.authoredRoom.walls.size();
  result.markerCount = result.markers.size();
  result.ok = true;
  result.status = "ascii_room_ok";
  result.reasonCode = "ascii_room_ok";
  return result;
}

}  // namespace iggy3d
