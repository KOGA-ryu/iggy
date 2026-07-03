#include "app/iggy3d/window/CreativeUiWindowFrame.hpp"

#include "app/iggy3d/window/CreativeWindowCoordinateSpace.hpp"

namespace iggy3d {

ProductCreativeUiFrame buildProductCreativeUiWindowFrame(
    const ProductCreativeUiWindowFrameRequest& request) {
  const ProductCreativeWindowCoordinateSpace coordinateSpace =
      resolveProductCreativeWindowCoordinateSpace(
          ProductCreativeWindowCoordinateSpaceRequest{
              request.logicalWidth,
              request.logicalHeight,
              request.drawableWidth,
              request.drawableHeight,
              request.fallbackWidth,
              request.fallbackHeight,
              1280,
              720});

  ProductCreativeUiFrameRequest frameRequest;
  frameRequest.window = request.window;
  frameRequest.facade = request.facade;
  frameRequest.virtualWidth = coordinateSpace.virtualWidth;
  frameRequest.virtualHeight = coordinateSpace.virtualHeight;
  frameRequest.theme = request.theme;
  return buildProductCreativeUiFrame(frameRequest);
}

}  // namespace iggy3d
