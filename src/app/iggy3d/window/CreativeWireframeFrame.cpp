#include "app/iggy3d/window/CreativeWireframeFrame.hpp"

#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/creative/Facade.hpp"

#include <string>
#include <utility>

namespace iggy3d {
namespace {

void setStatus(ProductCreativeWireframeFrameReceipt& receipt,
               std::string status) {
  receipt.status = std::move(status);
  receipt.reasonCode = receipt.status;
}

void copyWireframeReceipt(
    ProductCreativeWireframeFrameReceipt& receipt,
    const creative::CreativeDocumentWireframeReceipt& wireframe) {
  receipt.sourceAvailable = wireframe.sourceAvailable;
  receipt.objectCount = wireframe.objectCount;
  receipt.visibleObjectCount = wireframe.visibleObjectCount;
  receipt.itemCount = wireframe.itemCount;
  receipt.wireframeStatus = wireframe.status;
  receipt.wireframeReasonCode = std::string(wireframe.reasonCode);
}

void copySegmentReceipt(
    ProductCreativeWireframeFrameReceipt& receipt,
    const creative::CreativeDocumentWireframeSegmentReceipt& segments) {
  receipt.itemCount = segments.itemCount;
  receipt.boxItemCount = segments.boxItemCount;
  receipt.lineItemCount = segments.lineItemCount;
  receipt.pointItemCount = segments.pointItemCount;
  receipt.segmentCount = segments.segmentCount;
  receipt.skippedDegenerateCount = segments.skippedDegenerateCount;
  receipt.segmentStatus = segments.status;
  receipt.segmentReasonCode = std::string(segments.reasonCode);
}

}  // namespace

bool productCreativeWireframeFrameActiveForWindow(
    const ProductAppWindowState& window) noexcept {
  return window.interactionMode == ProductInteractionMode::Creative;
}

ProductCreativeWireframeFrameReceipt routeProductCreativeWireframeFrame(
    const ProductCreativeWireframeFrameRequest& request) {
  ProductCreativeWireframeFrameReceipt receipt;
  receipt.requested = true;

  if (request.window == nullptr) {
    setStatus(receipt, "product_creative_wireframe_frame_window_missing");
    return receipt;
  }

  receipt.active = productCreativeWireframeFrameActiveForWindow(*request.window);
  if (!receipt.active) {
    setStatus(receipt, "product_creative_wireframe_frame_inactive");
    return receipt;
  }

  receipt.facadeAvailable = request.facade != nullptr;
  if (request.facade == nullptr) {
    setStatus(receipt, "product_creative_wireframe_frame_facade_missing");
    return receipt;
  }

  const creative::CreativeDocument& document = request.facade->document();
  receipt.documentAvailable = document.isValid();
  if (!receipt.documentAvailable) {
    setStatus(receipt, "product_creative_wireframe_frame_document_invalid");
    return receipt;
  }

  const creative::CreativeDocumentWireframeBuildResult wireframe =
      creative::buildCreativeDocumentWireframeList(document,
                                                   request.projectionRequest);
  copyWireframeReceipt(receipt, wireframe.receipt);

  if (wireframe.receipt.status ==
      creative::CreativeDocumentWireframeStatus::InvalidProjection) {
    setStatus(receipt, "product_creative_wireframe_frame_invalid_projection");
    return receipt;
  }

  const creative::CreativeDocumentWireframeSegmentBuildResult segments =
      creative::buildCreativeDocumentWireframeSegments(wireframe.drawList);
  copySegmentReceipt(receipt, segments.receipt);

  if (receipt.objectCount == 0U) {
    setStatus(receipt, "product_creative_wireframe_frame_source_empty");
    return receipt;
  }

  if (receipt.segmentCount > 0U) {
    setStatus(receipt, "product_creative_wireframe_frame_built");
    return receipt;
  }

  setStatus(receipt, "product_creative_wireframe_frame_no_segments");
  return receipt;
}

}  // namespace iggy3d
