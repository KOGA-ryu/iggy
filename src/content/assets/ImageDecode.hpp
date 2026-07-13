#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace iggy3d {

inline constexpr std::uint32_t kStaticMeshImageMaximumDimension = 8192U;
inline constexpr std::uint64_t kStaticMeshImageMaximumPixelCount =
    16ULL * 1024ULL * 1024ULL;

struct DecodedImageRgba8 {
  std::vector<std::uint8_t> pixels;
  std::uint32_t width = 0;
  std::uint32_t height = 0;
  std::string reasonCode = "image_decode_not_requested";

  [[nodiscard]] bool ok() const noexcept {
    return width > 0U && height > 0U &&
           pixels.size() == static_cast<std::size_t>(width) * height * 4U;
  }
};

[[nodiscard]] DecodedImageRgba8 decodeImageRgba8(
    std::span<const std::uint8_t> encoded);

}  // namespace iggy3d
