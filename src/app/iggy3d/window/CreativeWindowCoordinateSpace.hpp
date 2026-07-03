#pragma once

#include <cstdint>
#include <string_view>

namespace iggy3d {

struct ProductCreativeWindowCoordinateSpaceRequest {
  std::uint32_t logicalWidth = 0;
  std::uint32_t logicalHeight = 0;
  std::uint32_t drawableWidth = 0;
  std::uint32_t drawableHeight = 0;
  std::uint32_t fallbackWidth = 1280;
  std::uint32_t fallbackHeight = 720;
  std::uint32_t guardWidth = 1280;
  std::uint32_t guardHeight = 720;
};

struct ProductCreativeWindowCoordinateSpace {
  std::uint32_t virtualWidth = 1280;
  std::uint32_t virtualHeight = 720;
  std::uint32_t drawableWidth = 0;
  std::uint32_t drawableHeight = 0;
  bool usedLogical = false;
  bool usedFallback = false;
  bool usedGuard = true;
  std::string_view status = "creative_coordinate_space_guard";
  std::string_view reasonCode = "creative_coordinate_space_guard";
};

[[nodiscard]] ProductCreativeWindowCoordinateSpace
resolveProductCreativeWindowCoordinateSpace(
    ProductCreativeWindowCoordinateSpaceRequest request) noexcept;

}  // namespace iggy3d
