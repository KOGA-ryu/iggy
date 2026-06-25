#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "app/iggy3d/AsciiRoomGrid.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/save/SaveEnvelope.hpp"

namespace iggy3d {

struct AsciiRoomCompileConfig {
  float tileSizeMeters = 1.0F;
  float floorThicknessMeters = 0.10F;
  float wallHeightMeters = 2.50F;
  float wallThicknessMeters = 1.0F;
  float markerYMeters = 0.05F;
  bool centerOnOrigin = true;
  std::int32_t storyIndex = 0;
  std::string roomId = "ascii_room";
  std::string sourceName;
};

struct AsciiRoomMarker {
  std::string id;
  std::string tag;
  char glyph = '\0';
  std::size_t row = 0;
  std::size_t column = 0;
  AsciiRoomWorldPosition worldPosition;
  std::size_t sourceLine = 1;
  std::size_t sourceColumn = 1;
};

struct AsciiRoomTerrainSurface {
  std::string floorId;
  AsciiRoomTerrainKind kind = AsciiRoomTerrainKind::Flat;
  std::vector<Vec3> topFacePoints;
  Vec3 normal{0.0F, 1.0F, 0.0F};
  bool ramp = false;
  bool blockedSlope = false;
};

struct AsciiRoomAuthoredRoomResult {
  bool ok = false;
  std::string status = "ascii_room_ok";
  std::string reasonCode = "ascii_room_ok";
  SaveAuthoredRoomSection authoredRoom;
  std::vector<AsciiRoomMarker> markers;
  std::vector<AsciiRoomTerrainSurface> terrainSurfaces;
  std::vector<AsciiRoomDiagnostic> diagnostics;
  std::size_t floorCount = 0;
  std::size_t wallCount = 0;
  std::size_t markerCount = 0;
  std::size_t elevatedFloorCount = 0;
  std::size_t rampCount = 0;
  std::size_t blockedSlopeCount = 0;
};

AsciiRoomAuthoredRoomResult compileAsciiRoomToAuthoredRoom(
    const AsciiRoomGrid& grid,
    const AsciiRoomCompileConfig& config = {});

}  // namespace iggy3d
