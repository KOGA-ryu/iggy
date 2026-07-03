#include "app/iggy3d/window/CreativeUiWindowFrame.hpp"

namespace iggy3d {
namespace {

[[nodiscard]] std::uint32_t effectiveExtent(std::uint32_t drawable,
                                            std::uint32_t fallback,
                                            std::uint32_t guard) noexcept {
  if (drawable > 0) {
    return drawable;
  }
  if (fallback > 0) {
    return fallback;
  }
  return guard;
}

}  // namespace

ProductCreativeUiFrame buildProductCreativeUiWindowFrame(
    const ProductCreativeUiWindowFrameRequest& request) {
  ProductCreativeUiFrameRequest frameRequest;
  frameRequest.window = request.window;
  frameRequest.facade = request.facade;
  frameRequest.virtualWidth =
      effectiveExtent(request.drawableWidth, request.fallbackWidth, 1280);
  frameRequest.virtualHeight =
      effectiveExtent(request.drawableHeight, request.fallbackHeight, 720);
  frameRequest.theme = request.theme;
  return buildProductCreativeUiFrame(frameRequest);
}

}  // namespace iggy3d
