#include "app/iggy3d/creative/bridge/WireframeFrame.hpp"

#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"
#include "app/iggy3d/creative/render/WireframeDebugLines.hpp"

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

void copyDebugLineReceipt(
    ProductCreativeWireframeFrameReceipt& receipt,
    const ProductCreativeWireframeDebugLineReceipt& debugLines) {
  receipt.debugLineRequested = debugLines.requested;
  receipt.debugLineSourceAvailable = debugLines.sourceAvailable;
  receipt.debugLineInputSegmentCount = debugLines.segmentCount;
  receipt.debugLineCount = debugLines.lineCount;
  receipt.debugLineSkippedDegenerateCount =
      debugLines.skippedDegenerateCount;
  receipt.debugLineStatus = debugLines.status;
  receipt.debugLineReasonCode = std::string(debugLines.reasonCode);
}

}  // namespace

bool productCreativeWireframeFrameActiveForWindow(
    const ProductAppWindowState& window) noexcept {
  return productCreativeDocumentEditorActiveForWindow(window);
}

ProductCreativeWireframeFrameReceipt routeProductCreativeWireframeFrame(
    const ProductCreativeWireframeFrameRequest& request) {
  return buildProductCreativeWireframeFrame(request).receipt;
}

ProductCreativeWireframeFrameBuildResult buildProductCreativeWireframeFrame(
    const ProductCreativeWireframeFrameRequest& request) {
  ProductCreativeWireframeFrameBuildResult result;
  ProductCreativeWireframeFrameReceipt& receipt = result.receipt;
  receipt.requested = true;

  if (request.window == nullptr) {
    setStatus(receipt, "product_creative_wireframe_frame_window_missing");
    return result;
  }

  receipt.active = productCreativeWireframeFrameActiveForWindow(*request.window);
  if (!receipt.active) {
    setStatus(receipt, "product_creative_wireframe_frame_inactive");
    return result;
  }

  creative::Facade* facade =
      request.creative != nullptr ? &request.creative->facade : nullptr;
  receipt.facadeAvailable = facade != nullptr;
  if (facade == nullptr) {
    setStatus(receipt, "product_creative_wireframe_frame_facade_missing");
    return result;
  }

  const creative::CreativeDocument& document = facade->document();
  receipt.documentAvailable = document.isValid();
  if (!receipt.documentAvailable) {
    setStatus(receipt, "product_creative_wireframe_frame_document_invalid");
    return result;
  }

  const creative::CreativeDocumentWireframeBuildResult wireframe =
      creative::buildCreativeDocumentWireframeList(document,
                                                   request.projectionRequest);
  copyWireframeReceipt(receipt, wireframe.receipt);

  if (wireframe.receipt.status ==
      creative::CreativeDocumentWireframeStatus::InvalidProjection) {
    setStatus(receipt, "product_creative_wireframe_frame_invalid_projection");
    return result;
  }

  const creative::CreativeDocumentWireframeSegmentBuildResult segments =
      creative::buildCreativeDocumentWireframeSegments(wireframe.drawList);
  copySegmentReceipt(receipt, segments.receipt);

  ProductCreativeWireframeDebugLineBuildResult debugLines =
      buildProductCreativeWireframeDebugLines(segments.segmentList);
  copyDebugLineReceipt(receipt, debugLines.receipt);
  result.debugLineList = std::move(debugLines.lineList);

  if (receipt.objectCount == 0U) {
    setStatus(receipt, "product_creative_wireframe_frame_source_empty");
    return result;
  }

  if (receipt.segmentCount > 0U) {
    setStatus(receipt, "product_creative_wireframe_frame_built");
    return result;
  }

  setStatus(receipt, "product_creative_wireframe_frame_no_segments");
  return result;
}

}  // namespace iggy3d
