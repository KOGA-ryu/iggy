#include "app/iggy3d/creative/bridge/UiInputFrame.hpp"

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

void setStatus(ProductCreativeUiDownstreamClickReceipt& receipt,
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
  receipt.clickX = request.click.x;
  receipt.clickY = request.click.y;
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

ProductCreativeUiDownstreamClickReceipt routeProductCreativeUiDownstreamClick(
    const ProductCreativeUiDownstreamClickRequest& request) {
  ProductCreativeUiDownstreamClickReceipt receipt;
  receipt.requested = true;
  receipt.clickPresent = request.click.clicked;
  receipt.creativeUiConsumed = request.creativeUiConsumed;
  receipt.higherPriorityUiConsumed = request.higherPriorityUiConsumed;
  receipt.downstreamClick = request.click;

  if (!request.click.clicked) {
    setStatus(receipt, "product_creative_ui_downstream_click_no_click");
    return receipt;
  }

  if (request.higherPriorityUiConsumed) {
    receipt.suppressed = true;
    receipt.downstreamClick.clicked = false;
    setStatus(receipt,
              "product_creative_ui_downstream_click_higher_priority");
    return receipt;
  }

  if (request.creativeUiConsumed) {
    receipt.suppressed = true;
    receipt.downstreamClick.clicked = false;
    setStatus(receipt, "product_creative_ui_downstream_click_suppressed");
    return receipt;
  }

  setStatus(receipt, "product_creative_ui_downstream_click_passthrough");
  return receipt;
}

}  // namespace iggy3d
