#include "app/iggy3d/room/ProductRoomVisualProof.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include "core/math/Aabb3.hpp"

namespace iggy3d {
namespace {

constexpr ProductPrimitiveColor kBackgroundColor{18, 20, 22};
constexpr ProductPrimitiveColor kFloorColor{54, 78, 68};
constexpr ProductPrimitiveColor kWallColor{76, 86, 92};

bool colorEquals(ProductPrimitiveColor lhs, ProductPrimitiveColor rhs) {
  return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b;
}

bool isFloorKind(ProductPrimitiveDrawKind kind) {
  // branch-gate: BG-1038
  switch (kind) {
    case ProductPrimitiveDrawKind::FloorTile:
    case ProductPrimitiveDrawKind::ElevatedFloorTile:
    case ProductPrimitiveDrawKind::RampTile:
    case ProductPrimitiveDrawKind::BlockedSlopeTile:
      return true;
    case ProductPrimitiveDrawKind::WallTile:
    case ProductPrimitiveDrawKind::PlayerMarker:
    case ProductPrimitiveDrawKind::NpcMarker:
    case ProductPrimitiveDrawKind::PickupMarker:
    case ProductPrimitiveDrawKind::InteractableMarker:
    case ProductPrimitiveDrawKind::ObjectiveMarker:
    case ProductPrimitiveDrawKind::TacticalMarker:
    case ProductPrimitiveDrawKind::DebugMarker:
    case ProductPrimitiveDrawKind::PlayerFocusIndicator:
    case ProductPrimitiveDrawKind::DoorMarker:
    case ProductPrimitiveDrawKind::RoomEditorCursor:
    case ProductPrimitiveDrawKind::RoomEditorPlacementPreview:
      return false;
  }
  return false;
}

bool isVisualProofRoomKind(ProductPrimitiveDrawKind kind) {
  return isFloorKind(kind) || kind == ProductPrimitiveDrawKind::WallTile;
}

bool validImageSize(std::uint32_t width, std::uint32_t height) {
  return width >= 16U && height >= 16U && width <= 4096U && height <= 4096U;
}

std::string sanitizedCommentValue(std::string_view value) {
  std::string sanitized;
  sanitized.reserve(value.size());
  for (const char character : value) {
    // branch-gate: BG-1038
    sanitized.push_back((character == '\n' || character == '\r') ? '_' : character);
  }
  return sanitized;
}

void appendComment(std::ofstream& output,
                   std::string_view key,
                   std::string_view value) {
  output << "# " << key << "=" << sanitizedCommentValue(value) << "\n";
}

void appendComment(std::ofstream& output,
                   std::string_view key,
                   std::uint64_t value) {
  output << "# " << key << "=" << value << "\n";
}

struct ProofBounds {
  float minX = std::numeric_limits<float>::max();
  float minZ = std::numeric_limits<float>::max();
  float maxX = -std::numeric_limits<float>::max();
  float maxZ = -std::numeric_limits<float>::max();
};

void includeBounds(ProofBounds& bounds, const Aabb3& itemBounds) {
  bounds.minX = std::min(bounds.minX, itemBounds.min.x);
  bounds.minZ = std::min(bounds.minZ, itemBounds.min.z);
  bounds.maxX = std::max(bounds.maxX, itemBounds.max.x);
  bounds.maxZ = std::max(bounds.maxZ, itemBounds.max.z);
}

bool validBounds(const ProofBounds& bounds) {
  return std::isfinite(bounds.minX) && std::isfinite(bounds.minZ) &&
         std::isfinite(bounds.maxX) && std::isfinite(bounds.maxZ) &&
         bounds.maxX > bounds.minX && bounds.maxZ > bounds.minZ;
}

float projectionScale(std::uint32_t size, std::uint32_t padding, float span) {
  // branch-gate: BG-1038
  const std::uint32_t drawable = size > padding * 2U ? size - padding * 2U : 1U;
  return static_cast<float>(drawable) / span;
}

std::uint32_t projectAxis(float value,
                          float minValue,
                          float scale,
                          std::uint32_t padding,
                          std::uint32_t maxPixel) {
  const float projected =
      static_cast<float>(padding) + (value - minValue) * scale;
  const float clamped =
      std::clamp(projected, 0.0F, static_cast<float>(maxPixel));
  return static_cast<std::uint32_t>(std::lround(clamped));
}

std::uint32_t projectZ(float value,
                       const ProofBounds& bounds,
                       float scale,
                       std::uint32_t padding,
                       std::uint32_t maxPixel) {
  const float projected =
      static_cast<float>(maxPixel - padding) - (value - bounds.minZ) * scale;
  const float clamped =
      std::clamp(projected, 0.0F, static_cast<float>(maxPixel));
  return static_cast<std::uint32_t>(std::lround(clamped));
}

struct Raster {
  std::uint32_t width = 0;
  std::uint32_t height = 0;
  std::vector<ProductPrimitiveColor> pixels;
};

void fillRect(Raster& raster,
              std::uint32_t minX,
              std::uint32_t minY,
              std::uint32_t maxX,
              std::uint32_t maxY,
              ProductPrimitiveColor color) {
  const std::uint32_t x0 = std::min(minX, maxX);
  const std::uint32_t x1 = std::max(minX, maxX);
  const std::uint32_t y0 = std::min(minY, maxY);
  const std::uint32_t y1 = std::max(minY, maxY);
  for (std::uint32_t y = y0; y <= y1; ++y) {
    for (std::uint32_t x = x0; x <= x1; ++x) {
      raster.pixels[static_cast<std::size_t>(y) * raster.width + x] = color;
    }
  }
}

void drawRoomItems(const ProductPrimitiveDrawList& drawList,
                   const ProofBounds& bounds,
                   bool drawWalls,
                   Raster& raster,
                   std::uint32_t padding) {
  const float scaleX =
      projectionScale(raster.width, padding, bounds.maxX - bounds.minX);
  const float scaleZ =
      projectionScale(raster.height, padding, bounds.maxZ - bounds.minZ);
  const float scale = std::min(scaleX, scaleZ);
  const std::uint32_t maxX = raster.width - 1U;
  const std::uint32_t maxY = raster.height - 1U;

  for (const ProductPrimitiveDrawItem& item : drawList.items) {
    const bool itemIsWall = item.kind == ProductPrimitiveDrawKind::WallTile;
    // branch-gate: BG-1038
    const bool itemMatches = drawWalls ? itemIsWall : isFloorKind(item.kind);
    // branch-gate: BG-1038
    if (!item.visible || !itemMatches || !isValid(item.worldBounds)) {
      continue;
    }
    const std::uint32_t x0 =
        projectAxis(item.worldBounds.min.x, bounds.minX, scale, padding, maxX);
    const std::uint32_t x1 =
        projectAxis(item.worldBounds.max.x, bounds.minX, scale, padding, maxX);
    const std::uint32_t y0 =
        projectZ(item.worldBounds.max.z, bounds, scale, padding, maxY);
    const std::uint32_t y1 =
        projectZ(item.worldBounds.min.z, bounds, scale, padding, maxY);
    // branch-gate: BG-1038
    fillRect(raster, x0, y0, x1, y1, drawWalls ? kWallColor : kFloorColor);
  }
}

void countRoomPixels(const Raster& raster, ProductRoomVisualProofResult& result) {
  for (const ProductPrimitiveColor pixel : raster.pixels) {
    // branch-gate: BG-1038
    if (colorEquals(pixel, kFloorColor)) {
      ++result.floorPixelCount;
    }
    // branch-gate: BG-1038
    if (colorEquals(pixel, kWallColor)) {
      ++result.wallPixelCount;
    }
  }
}

bool writePpm(const Raster& raster,
              const ProductRoomVisualProofResult& result,
              const std::filesystem::path& path) {
  std::ofstream output(path, std::ios::binary);
  // branch-gate: BG-1038
  if (!output) {
    return false;
  }

  output << "P6\n";
  output << "# iggy3d_room_visual_proof_v1\n";
  appendComment(output, "room_id", result.metadata.roomId);
  appendComment(output, "active_room_source", result.metadata.activeRoomSource);
  appendComment(output, "authored_floor_count",
                result.metadata.authoredFloorCount);
  appendComment(output, "authored_wall_count",
                result.metadata.authoredWallCount);
  appendComment(output, "optimized_wall_draw_count",
                result.metadata.optimizedWallDrawCount);
  appendComment(output, "room_geometry_signature",
                result.metadata.roomGeometrySignature);
  appendComment(output, "floor_pixel_count", result.floorPixelCount);
  appendComment(output, "wall_pixel_count", result.wallPixelCount);
  output << raster.width << " " << raster.height << "\n255\n";
  for (const ProductPrimitiveColor pixel : raster.pixels) {
    const std::array<char, 3> bytes{
        static_cast<char>(pixel.r),
        static_cast<char>(pixel.g),
        static_cast<char>(pixel.b),
    };
    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  }
  return static_cast<bool>(output);
}

ProductRoomVisualProofResult rejectedResult(
    const ProductRoomVisualProofRequest& request,
    std::string status,
    std::string reasonCode) {
  ProductRoomVisualProofResult result;
  result.status = std::move(status);
  result.reasonCode = std::move(reasonCode);
  result.outputPath = request.outputPath;
  result.width = request.width;
  result.height = request.height;
  result.metadata = request.metadata;
  return result;
}

}  // namespace

ProductRoomVisualProofResult writeProductRoomVisualProofPpm(
    const ProductRoomVisualProofRequest& request) {
  // branch-gate: BG-1038
  if (request.drawList == nullptr) {
    return rejectedResult(request, "room_visual_proof_missing_draw_list",
                          "room_visual_proof_missing_draw_list");
  }
  // branch-gate: BG-1038
  if (request.outputPath.empty()) {
    return rejectedResult(request, "room_visual_proof_missing_output",
                          "room_visual_proof_missing_output");
  }
  // branch-gate: BG-1038
  if (!validImageSize(request.width, request.height)) {
    return rejectedResult(request, "room_visual_proof_invalid_size",
                          "room_visual_proof_invalid_size");
  }

  ProductRoomVisualProofResult result;
  result.outputPath = request.outputPath;
  result.width = request.width;
  result.height = request.height;
  result.metadata = request.metadata;

  ProofBounds bounds;
  for (const ProductPrimitiveDrawItem& item : request.drawList->items) {
    // branch-gate: BG-1038
    if (!item.visible || !isVisualProofRoomKind(item.kind) ||
        !isValid(item.worldBounds)) {
      continue;
    }
    includeBounds(bounds, item.worldBounds);
    ++result.roomItemCount;
    // branch-gate: BG-1038
    if (item.kind == ProductPrimitiveDrawKind::WallTile) {
      ++result.wallItemCount;
    } else {
      ++result.floorItemCount;
    }
  }

  // branch-gate: BG-1038
  if (!validBounds(bounds) || result.floorItemCount == 0U ||
      result.wallItemCount == 0U) {
    result.status = "room_visual_proof_missing_room_geometry";
    result.reasonCode = "room_visual_proof_missing_room_geometry";
    return result;
  }

  Raster raster;
  raster.width = request.width;
  raster.height = request.height;
  raster.pixels.assign(
      static_cast<std::size_t>(request.width) * request.height,
      kBackgroundColor);
  drawRoomItems(*request.drawList, bounds, false, raster, request.paddingPixels);
  drawRoomItems(*request.drawList, bounds, true, raster, request.paddingPixels);
  countRoomPixels(raster, result);

  // branch-gate: BG-1038
  if (result.floorPixelCount == 0U || result.wallPixelCount == 0U) {
    result.status = "room_visual_proof_empty_pixels";
    result.reasonCode = "room_visual_proof_empty_pixels";
    return result;
  }

  // branch-gate: BG-1038
  if (!writePpm(raster, result, request.outputPath)) {
    result.status = "room_visual_proof_write_failed";
    result.reasonCode = "room_visual_proof_write_failed";
    return result;
  }

  result.ok = true;
  result.status = "room_visual_proof_written";
  result.reasonCode = "room_visual_proof_written";
  return result;
}

}  // namespace iggy3d
