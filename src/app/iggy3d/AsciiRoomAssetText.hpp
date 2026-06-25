#pragma once

#include <cstddef>
#include <string>

#include "content/assets/RoomAsset.hpp"

namespace iggy3d {

struct AsciiRoomAssetTextConfig {
  float feetToMeters = 0.3048F;
  int decimalPlaces = 6;
  bool includeConversionTable = true;
};

struct AsciiRoomAssetTextResult {
  bool ok = false;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  std::string text;
  std::size_t staticMeshCount = 0;
  std::size_t anchorCount = 0;
  std::size_t spatialSurfaceCount = 0;
};

AsciiRoomAssetTextResult writeAsciiRoomAssetText(
    const RoomAsset& room,
    const AsciiRoomAssetTextConfig& config = {});

}  // namespace iggy3d
