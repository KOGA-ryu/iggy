#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

#include "app/iggy3d/view/PrimitiveDrawList.hpp"

namespace iggy3d {

struct ProductRoomVisualProofMetadata {
  std::string roomId = "none";
  std::string activeRoomSource = "none";
  std::uint64_t authoredFloorCount = 0;
  std::uint64_t authoredWallCount = 0;
  std::uint64_t optimizedWallDrawCount = 0;
  std::uint64_t roomGeometrySignature = 0;
};

struct ProductRoomVisualProofRequest {
  const ProductPrimitiveDrawList* drawList = nullptr;
  ProductRoomVisualProofMetadata metadata;
  std::filesystem::path outputPath;
  std::uint32_t width = 256;
  std::uint32_t height = 256;
  std::uint32_t paddingPixels = 8;
};

struct ProductRoomVisualProofResult {
  bool ok = false;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  std::filesystem::path outputPath;
  std::uint32_t width = 0;
  std::uint32_t height = 0;
  std::uint64_t floorPixelCount = 0;
  std::uint64_t wallPixelCount = 0;
  std::uint64_t roomItemCount = 0;
  std::uint64_t floorItemCount = 0;
  std::uint64_t wallItemCount = 0;
  ProductRoomVisualProofMetadata metadata;
};

ProductRoomVisualProofResult writeProductRoomVisualProofPpm(
    const ProductRoomVisualProofRequest& request);

}  // namespace iggy3d
