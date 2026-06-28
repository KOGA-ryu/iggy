#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "app/iggy3d/ascii_room/AsciiRoomAssetText.hpp"
#include "app/iggy3d/ascii_room/AsciiRoomGrid.hpp"
#include "app/iggy3d/ascii_room/AsciiRoomToAuthoredRoom.hpp"
#include "app/iggy3d/ascii_room/AsciiRoomToRoomAsset.hpp"

namespace iggy3d {

struct ProductAsciiRoomAuthoringRequest {
  std::string sourceText;
  std::string sourceName = "inline_ascii_room";
  std::string roomId = "ascii_room";
  std::string sourceSubset = "ascii_room_authoring";
  float tileSizeMeters = 1.0F;
  float floorThicknessMeters = 0.10F;
  float wallHeightMeters = 2.50F;
  float wallThicknessMeters = 1.0F;
  float markerYMeters = 0.05F;
  bool centerOnOrigin = true;
  std::int32_t storyIndex = 0;
  bool emitAssetText = true;
  AsciiRoomAssetTextConfig assetTextConfig;
};

struct ProductAsciiRoomAuthoringResult {
  bool ok = false;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  std::string failedStage = "not_started";
  AsciiRoomSource source;
  AsciiRoomGridBuildResult grid;
  AsciiRoomAuthoredRoomResult authoredRoom;
  AsciiRoomToRoomAssetResult roomAsset;
  AsciiRoomAssetTextResult assetText;
  std::vector<AsciiRoomDiagnostic> diagnostics;
  std::size_t width = 0;
  std::size_t height = 0;
  std::size_t floorCount = 0;
  std::size_t wallCount = 0;
  std::size_t objectCount = 0;
  std::size_t markerCount = 0;
  std::size_t elevatedFloorCount = 0;
  std::size_t rampCount = 0;
  std::size_t blockedSlopeCount = 0;
  std::size_t staticMeshCount = 0;
  std::size_t anchorCount = 0;
  std::size_t spatialSurfaceCount = 0;
};

ProductAsciiRoomAuthoringResult buildProductAsciiRoomAuthoring(
    const ProductAsciiRoomAuthoringRequest& request);

}  // namespace iggy3d
