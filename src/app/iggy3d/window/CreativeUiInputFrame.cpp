#include "app/iggy3d/window/CreativeUiInputFrame.hpp"

#include <utility>

namespace iggy3d {
namespace {

void copyRouteFields(ProductCreativeUiInputFrameReceipt& receipt,
                     const ProductUiHitRouteReceipt& route) {
  receipt.hit = route.hit;
  receipt.consumed = route.consumed;
  receipt.enabled = route.enabled;
  receipt.surface = route.surface;
  receipt.kind = route.kind;
  receipt.action = route.action;
  receipt.layerIndex = route.layerIndex;
  receipt.regionIndex = route.regionIndex;
  receipt.semanticId = route.semanticId;
}

void setStatus(ProductCreativeUiInputFrameReceipt& receipt,
               std::string status) {
  receipt.status = std::move(status);
  receipt.reasonCode = receipt.status;
}

}  // namespace

ProductCreativeUiInputFrameReceipt routeProductCreativeUiInputFrame(
    const ProductCreativeUiInputFrameRequest& request) {
  ProductCreativeUiInputFrameReceipt receipt;
  receipt.requested = true;
  receipt.clickPresent = request.click.clicked;
  receipt.drawListAvailable = request.creativeUiDrawList != nullptr;

  if (!request.click.clicked) {
    setStatus(receipt, "product_creative_ui_input_no_click");
    return receipt;
  }

  if (request.creativeUiDrawList == nullptr) {
    setStatus(receipt, "product_creative_ui_input_draw_list_missing");
    return receipt;
  }

  const ProductUiHitLayer layer{
      ProductUiHitSurface::CreativeOverlay,
      request.creativeUiDrawList,
      true,
  };
  const ProductUiHitRouteReceipt route = routeProductUiHit(
      ProductUiHitRouteRequest{&layer, 1U, request.click.x, request.click.y});
  receipt.routed = true;
  copyRouteFields(receipt, route);

  if (!request.creativeUiDrawList->ready) {
    setStatus(receipt, "product_creative_ui_input_not_ready");
    return receipt;
  }

  if (route.consumed) {
    setStatus(receipt, "product_creative_ui_input_consumed");
    return receipt;
  }

  if (route.hit) {
    setStatus(receipt, "product_creative_ui_input_hit_disabled");
    return receipt;
  }

  setStatus(receipt, "product_creative_ui_input_miss");
  return receipt;
}

}  // namespace iggy3d
