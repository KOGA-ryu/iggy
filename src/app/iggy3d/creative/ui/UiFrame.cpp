#include "app/iggy3d/creative/ui/UiFrame.hpp"

#include "app/iggy3d/menu/FrontendRouter.hpp"

namespace iggy3d {
namespace {

ProductCreativeUiProjectionReceipt makeFrameProjectionReceipt(
    const ProductCreativeUiFrameRequest& request,
    bool requested,
    std::string_view status) noexcept {
  ProductCreativeUiProjectionReceipt receipt;
  receipt.requested = requested;
  receipt.ready = false;
  receipt.partial = false;
  receipt.status = status;
  receipt.reasonCode = status;
  receipt.usedFacade = false;
  receipt.usedModel = false;
  receipt.virtualWidth = request.virtualWidth;
  receipt.virtualHeight = request.virtualHeight;
  receipt.theme = request.theme;
  return receipt;
}

ProductCreativeUiFrameReceipt frameReceiptFromProjection(
    const ProductCreativeUiProjectionReceipt& projectionReceipt) noexcept {
  ProductCreativeUiFrameReceipt receipt;
  receipt.requested = projectionReceipt.requested;
  receipt.active = true;
  receipt.projected = true;
  receipt.recorded = true;
  receipt.ready = projectionReceipt.ready;
  receipt.partial = projectionReceipt.partial;
  receipt.status = projectionReceipt.status;
  receipt.reasonCode = projectionReceipt.reasonCode;
  receipt.primitiveCount = projectionReceipt.primitiveCount;
  receipt.textCount = projectionReceipt.textCount;
  receipt.rectCount = projectionReceipt.rectCount;
  receipt.rowCount = projectionReceipt.rowCount;
  receipt.hitRegionCount = projectionReceipt.hitRegionCount;
  return receipt;
}

}  // namespace

bool productCreativeUiActiveForWindow(
    const ProductAppWindowState& window) noexcept {
  return productCreativeDocumentEditorActiveForWindow(window);
}

ProductCreativeUiFrame buildProductCreativeUiFrame(
    const ProductCreativeUiFrameRequest& request) {
  ProductCreativeUiFrame frame;

  if (request.window == nullptr) {
    frame.receipt.status = "product_creative_ui_frame_window_missing";
    frame.receipt.reasonCode = "product_creative_ui_frame_window_missing";
    return frame;
  }

  ProductAppWindowState& window = *request.window;
  if (!productCreativeUiActiveForWindow(window)) {
    frame.projection.receipt = makeFrameProjectionReceipt(
        request, false, "product_creative_ui_frame_inactive");
    recordProductCreativeUiProjection(window, frame.projection.receipt);
    frame.receipt.recorded = true;
    frame.receipt.status = "product_creative_ui_frame_inactive";
    frame.receipt.reasonCode = "product_creative_ui_frame_inactive";
    return frame;
  }

  if (request.creative == nullptr) {
    frame.projection.receipt = makeFrameProjectionReceipt(
        request, true, "product_creative_ui_frame_facade_missing");
    recordProductCreativeUiProjection(window, frame.projection.receipt);
    frame.receipt.requested = true;
    frame.receipt.active = true;
    frame.receipt.recorded = true;
    frame.receipt.status = "product_creative_ui_frame_facade_missing";
    frame.receipt.reasonCode = "product_creative_ui_frame_facade_missing";
    return frame;
  }

  ProductCreativeUiProjectionRequest projectionRequest;
  projectionRequest.creative = request.creative;
  projectionRequest.virtualWidth = request.virtualWidth;
  projectionRequest.virtualHeight = request.virtualHeight;
  projectionRequest.theme = request.theme;
  frame.projection = buildProductCreativeUiProjection(projectionRequest);
  recordProductCreativeUiProjection(window, frame.projection.receipt);
  frame.receipt = frameReceiptFromProjection(frame.projection.receipt);
  return frame;
}

}  // namespace iggy3d
