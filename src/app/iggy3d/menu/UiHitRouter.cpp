#include "app/iggy3d/menu/UiHitRouter.hpp"

namespace iggy3d {
namespace {

[[nodiscard]] bool pointInsideRect(float x,
                                   float y,
                                   ProductUiRect rect) noexcept {
  return x >= rect.x && y >= rect.y && x < rect.x + rect.width &&
         y < rect.y + rect.height;
}

void recordHit(ProductUiHitRouteReceipt& receipt,
               const ProductUiHitLayer& layer,
               const UiHitRegion& region,
               std::size_t layerIndex,
               std::size_t regionIndex) {
  receipt.hit = true;
  receipt.consumed = region.enabled;
  receipt.enabled = region.enabled;
  receipt.surface = layer.surface;
  receipt.kind = region.kind;
  receipt.action = region.action;
  receipt.layerIndex = layerIndex;
  receipt.regionIndex = regionIndex;
  receipt.semanticId = region.semanticId;
  receipt.message = region.enabled ? "ui_hit_consumed" : "ui_hit_disabled";
}

}  // namespace

ProductUiHitRouteReceipt routeProductUiHit(
    const ProductUiHitRouteRequest& request) {
  ProductUiHitRouteReceipt receipt;
  receipt.requested = true;

  if (request.layers == nullptr || request.layerCount == 0U) {
    receipt.message = "ui_hit_no_layers";
    return receipt;
  }

  for (std::size_t layerIndex = 0; layerIndex < request.layerCount;
       ++layerIndex) {
    const ProductUiHitLayer& layer = request.layers[layerIndex];
    if (!layer.enabled || layer.drawList == nullptr || !layer.drawList->ready) {
      continue;
    }

    const ProductUiDrawList& drawList = *layer.drawList;
    for (std::size_t reverseIndex = drawList.hitRegions.size();
         reverseIndex > 0U;
         --reverseIndex) {
      const std::size_t regionIndex = reverseIndex - 1U;
      const UiHitRegion& region = drawList.hitRegions[regionIndex];
      if (!pointInsideRect(request.x, request.y, region.rect)) {
        continue;
      }

      recordHit(receipt, layer, region, layerIndex, regionIndex);
      return receipt;
    }
  }

  receipt.message = "ui_hit_miss";
  return receipt;
}

}  // namespace iggy3d
