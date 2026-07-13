#include "content/assets/ImageDecode.hpp"

#include <climits>
#include <cstddef>
#include <limits>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-qual"
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wdouble-promotion"
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wsign-conversion"
#endif
#define STBI_NO_STDIO
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_MAX_DIMENSIONS 8192
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

namespace iggy3d {

DecodedImageRgba8 decodeImageRgba8(
    std::span<const std::uint8_t> encoded) {
  DecodedImageRgba8 result;
  if (encoded.empty() || encoded.size() > static_cast<std::size_t>(INT_MAX)) {
    result.reasonCode = "image_encoded_size_invalid";
    return result;
  }

  int width = 0;
  int height = 0;
  int sourceChannels = 0;
  if (stbi_info_from_memory(encoded.data(), static_cast<int>(encoded.size()),
                            &width, &height, &sourceChannels) == 0 ||
      width <= 0 || height <= 0 ||
      static_cast<std::uint32_t>(width) >
          kStaticMeshImageMaximumDimension ||
      static_cast<std::uint32_t>(height) >
          kStaticMeshImageMaximumDimension ||
      static_cast<std::uint64_t>(width) *
              static_cast<std::uint64_t>(height) >
          kStaticMeshImageMaximumPixelCount) {
    result.reasonCode = "image_dimensions_invalid";
    return result;
  }

  stbi_uc* decoded = stbi_load_from_memory(
      encoded.data(), static_cast<int>(encoded.size()), &width, &height,
      &sourceChannels, STBI_rgb_alpha);
  if (decoded == nullptr) {
    result.reasonCode = "image_decode_failed";
    return result;
  }

  const std::size_t byteCount = static_cast<std::size_t>(width) *
                                static_cast<std::size_t>(height) * 4U;
  result.pixels.assign(decoded, decoded + byteCount);
  stbi_image_free(decoded);
  result.width = static_cast<std::uint32_t>(width);
  result.height = static_cast<std::uint32_t>(height);
  result.reasonCode = "image_decoded_rgba8";
  return result;
}

}  // namespace iggy3d
