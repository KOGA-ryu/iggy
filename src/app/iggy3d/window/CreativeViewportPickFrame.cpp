#include "app/iggy3d/window/CreativeViewportPickFrame.hpp"

#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"

#include <span>
#include <utility>

namespace iggy3d {
namespace {

void setStatus(ProductCreativeViewportPickFrameReceipt& receipt,
               std::string status) {
  receipt.status = std::move(status);
  receipt.reasonCode = receipt.status;
}

void copyPickReceipt(ProductCreativeViewportPickFrameReceipt& receipt,
                     const creative::CreativeViewportPickReceipt& pick) {
  receipt.picked = pick.hit;
  receipt.pickStatus = pick.status;
  receipt.coord = pick.coord;
  receipt.gridIndex = pick.index;
  receipt.objectId = pick.objectId;
  receipt.objectKind = pick.objectKind;
  receipt.occupancyKind = pick.occupancyKind;
  receipt.target = pick.target;
  receipt.cellIndex = pick.cellIndex;
  receipt.pickMessage = pick.message;
}

[[nodiscard]] bool shouldRunPickerForEmptyProjectionCells(
    const ProductCreativeViewportPickFrameRequest& request) noexcept {
  return !creative::isValidGridSize(request.projectionRequest.gridSize) ||
         !creative::isValidCreativeViewportPickViewport(request.viewport);
}

}  // namespace

bool productCreativeViewportPickActiveForWindow(
    const ProductAppWindowState& window) noexcept {
  return productCreativeDocumentEditorActiveForWindow(window);
}

ProductCreativeViewportPickFrameReceipt routeProductCreativeViewportPickFrame(
    const ProductCreativeViewportPickFrameRequest& request) {
  ProductCreativeViewportPickFrameReceipt receipt;
  receipt.requested = true;
  receipt.clickPresent = request.click.clicked;
  receipt.downstreamClickSuppressed = request.downstreamClickSuppressed;

  if (request.window == nullptr) {
    setStatus(receipt, "product_creative_viewport_pick_window_missing");
    return receipt;
  }

  receipt.active = productCreativeViewportPickActiveForWindow(*request.window);
  if (!receipt.active) {
    setStatus(receipt, "product_creative_viewport_pick_inactive");
    return receipt;
  }

  if (!request.click.clicked) {
    setStatus(receipt, "product_creative_viewport_pick_no_click");
    return receipt;
  }

  if (request.downstreamClickSuppressed) {
    setStatus(receipt, "product_creative_viewport_pick_click_suppressed");
    return receipt;
  }

  receipt.facadeAvailable = request.facade != nullptr;
  if (request.facade == nullptr) {
    setStatus(receipt, "product_creative_viewport_pick_facade_missing");
    return receipt;
  }

  const std::span<const creative::CreativeObject> objects =
      request.facade->document().objects();
  receipt.objectCount = objects.size();
  receipt.sourceAvailable = !objects.empty();
  if (objects.empty()) {
    setStatus(receipt, "product_creative_viewport_pick_source_empty");
    return receipt;
  }

  creative::CreativeSpatialProjectionReceipt projection =
      creative::projectObjectsToGrid(objects, request.projectionRequest);
  receipt.projected =
      projection.status == creative::CreativeSpatialProjectionStatus::Projected;
  receipt.projectionCellCount = projection.cells.size();

  if (projection.cells.empty() &&
      !shouldRunPickerForEmptyProjectionCells(request)) {
    setStatus(receipt, "product_creative_viewport_pick_projection_empty");
    return receipt;
  }

  const creative::CreativeViewportPickReceipt pick =
      creative::pickCreativeViewportCell(creative::CreativeViewportPickRequest{
          request.viewport,
          request.projectionRequest.gridSize,
          projection.cells.data(),
          projection.cells.size(),
          request.click.x,
          request.click.y,
          request.z,
          request.depthMode,
      });
  copyPickReceipt(receipt, pick);

  if (pick.hit) {
    setStatus(receipt, "product_creative_viewport_pick_hit");
    return receipt;
  }

  setStatus(receipt, "product_creative_viewport_pick_" + pick.message);
  return receipt;
}

}  // namespace iggy3d
