#include "app/iggy3d/creative/bridge/UiWindowFrame.hpp"

#include "app/iggy3d/creative/bridge/WindowCoordinateSpace.hpp"

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
  frameRequest.creative = request.creative;
  frameRequest.virtualWidth = coordinateSpace.virtualWidth;
  frameRequest.virtualHeight = coordinateSpace.virtualHeight;
  frameRequest.theme = request.theme;
  return buildProductCreativeUiFrame(frameRequest);
}

ProductCreativeUiOverlayInputAvailability
resolveProductCreativeUiOverlayInputAvailability(
    const ProductCreativeUiOverlayInputAvailabilityRequest& request) noexcept {
  ProductCreativeUiOverlayInputAvailability result;
  result.requested = true;
  result.drawListAvailable = request.drawList != nullptr;
  result.drawListReady =
      request.drawList != nullptr && request.drawList->ready;
  result.rendererCanRenderCreativeOverlay =
      request.rendererCanRenderCreativeOverlay;
  result.windowDrawable = request.windowDrawable;
  result.drawableAvailable =
      request.drawableWidth > 0U && request.drawableHeight > 0U;
  result.gameplayOverlayRenderable = request.gameplayOverlayRenderable;

  if (!result.drawListAvailable) {
    result.status = "product_creative_ui_overlay_input_draw_list_missing";
    result.reasonCode = "product_creative_ui_overlay_input_draw_list_missing";
    return result;
  }
  if (!result.drawListReady) {
    result.status = "product_creative_ui_overlay_input_draw_list_not_ready";
    result.reasonCode = "product_creative_ui_overlay_input_draw_list_not_ready";
    return result;
  }
  if (!result.rendererCanRenderCreativeOverlay) {
    result.status = "product_creative_ui_overlay_input_renderer_unavailable";
    result.reasonCode = "product_creative_ui_overlay_input_renderer_unavailable";
    return result;
  }
  if (!result.windowDrawable) {
    result.status = "product_creative_ui_overlay_input_window_not_drawable";
    result.reasonCode = "product_creative_ui_overlay_input_window_not_drawable";
    return result;
  }
  if (!result.drawableAvailable) {
    result.status = "product_creative_ui_overlay_input_drawable_unavailable";
    result.reasonCode = "product_creative_ui_overlay_input_drawable_unavailable";
    return result;
  }
  if (!result.gameplayOverlayRenderable) {
    result.status = "product_creative_ui_overlay_input_gameplay_unavailable";
    result.reasonCode = "product_creative_ui_overlay_input_gameplay_unavailable";
    return result;
  }

  result.inputAvailable = true;
  result.status = "product_creative_ui_overlay_input_available";
  result.reasonCode = "product_creative_ui_overlay_input_available";
  return result;
}

}  // namespace iggy3d
