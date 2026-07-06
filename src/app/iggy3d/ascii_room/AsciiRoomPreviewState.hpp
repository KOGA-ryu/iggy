#pragma once

#include <cstdint>
#include <string>

namespace iggy3d {

// Owned ascii-room preview state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: ascii_room. Behavior-identical.
struct ProductAsciiRoomPreviewState {
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  std::string failedStage = "not_started";
  std::string roomId = "none";
  std::string sourceName = "none";
  bool ready = false;
  std::uint64_t width = 0;
  std::uint64_t height = 0;
  std::uint64_t floorCount = 0;
  std::uint64_t wallCount = 0;
  std::uint64_t objectCount = 0;
  std::uint64_t markerCount = 0;
  std::uint64_t elevatedFloorCount = 0;
  std::uint64_t rampCount = 0;
  std::uint64_t blockedSlopeCount = 0;
  std::uint64_t staticMeshCount = 0;
  std::uint64_t anchorCount = 0;
  std::uint64_t spatialSurfaceCount = 0;
  bool assetTextWritten = false;
  std::uint64_t assetTextBytes = 0;
};

}  // namespace iggy3d
