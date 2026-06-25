#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "app/iggy3d/AsciiRoomToAuthoredRoom.hpp"
#include "content/assets/RoomAsset.hpp"

namespace iggy3d {

struct AsciiRoomToRoomAssetConfig {
  std::string roomId = "ascii_room";
  std::string sourceName;
  std::string units = "m";
  std::string source = "iggy3d.ascii_room";
  std::string sourceSubset = "ascii_room_authoring";
  std::string floorMeshId = "floor_rect";
  std::string wallMeshId = "wall_segment";
  std::string doorMeshId = "door_panel";
  std::string floorRole = "floor";
  std::string wallRole = "wall";
  std::string doorRole = "door";
  float tileSizeMeters = 1.0F;
  float wallHeightMeters = 2.5F;
};

struct AsciiRoomToRoomAssetResult {
  bool ok = false;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  RoomAsset room;
  std::vector<AsciiRoomDiagnostic> diagnostics;
  std::size_t staticMeshCount = 0;
  std::size_t anchorCount = 0;
  std::size_t spatialSurfaceCount = 0;
  std::size_t walkableSurfaceCount = 0;
  std::size_t actorBlockerSurfaceCount = 0;
  std::size_t projectileBlockerSurfaceCount = 0;
};

AsciiRoomToRoomAssetResult buildRoomAssetFromAsciiRoom(
    const AsciiRoomAuthoredRoomResult& authored,
    const AsciiRoomToRoomAssetConfig& config = {});

}  // namespace iggy3d
