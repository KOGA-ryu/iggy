#include "app/iggy3d/creative/bridge/WindowCoordinateSpace.hpp"

namespace iggy3d {
namespace {

[[nodiscard]] bool validExtent(std::uint32_t width,
                               std::uint32_t height) noexcept {
  return width > 0U && height > 0U;
}

}  // namespace

ProductCreativeWindowCoordinateSpace
resolveProductCreativeWindowCoordinateSpace(
    ProductCreativeWindowCoordinateSpaceRequest request) noexcept {
  ProductCreativeWindowCoordinateSpace result;
  result.drawableWidth = request.drawableWidth;
  result.drawableHeight = request.drawableHeight;

  if (validExtent(request.logicalWidth, request.logicalHeight)) {
    result.virtualWidth = request.logicalWidth;
    result.virtualHeight = request.logicalHeight;
    result.usedLogical = true;
    result.usedFallback = false;
    result.usedGuard = false;
    result.status = "creative_coordinate_space_logical";
    result.reasonCode = "creative_coordinate_space_logical";
    return result;
  }

  if (validExtent(request.fallbackWidth, request.fallbackHeight)) {
    result.virtualWidth = request.fallbackWidth;
    result.virtualHeight = request.fallbackHeight;
    result.usedLogical = false;
    result.usedFallback = true;
    result.usedGuard = false;
    result.status = "creative_coordinate_space_fallback";
    result.reasonCode = "creative_coordinate_space_fallback";
    return result;
  }

  result.virtualWidth = validExtent(request.guardWidth, request.guardHeight)
                            ? request.guardWidth
                            : 1280U;
  result.virtualHeight = validExtent(request.guardWidth, request.guardHeight)
                             ? request.guardHeight
                             : 720U;
  result.usedLogical = false;
  result.usedFallback = false;
  result.usedGuard = true;
  result.status = "creative_coordinate_space_guard";
  result.reasonCode = "creative_coordinate_space_guard";
  return result;
}

}  // namespace iggy3d
