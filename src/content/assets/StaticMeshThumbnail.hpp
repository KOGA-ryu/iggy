#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace iggy3d {

struct StaticMeshAsset;

inline constexpr std::size_t kStaticMeshThumbnailExtent = 32U;
inline constexpr std::size_t kStaticMeshThumbnailPixelCount =
    kStaticMeshThumbnailExtent * kStaticMeshThumbnailExtent;

struct StaticMeshThumbnailPixel {
  std::uint8_t r = 0U;
  std::uint8_t g = 0U;
  std::uint8_t b = 0U;
  std::uint8_t a = 0U;
};

struct StaticMeshAssetThumbnail {
  std::array<StaticMeshThumbnailPixel, kStaticMeshThumbnailPixelCount> pixels{};
  std::uint16_t coveredPixelCount = 0U;
  bool valid = false;
};

// Deterministic isometric preview generated once during asset discovery. The
// fill comes from imported triangles; cyan bounds, amber collision, and socket
// markers keep authoring facts visible without a per-frame mesh bake.
[[nodiscard]] StaticMeshAssetThumbnail buildStaticMeshAssetThumbnail(
    const StaticMeshAsset& asset) noexcept;

}  // namespace iggy3d
