#include "app/iggy3d/menu/CreativeUiProjection.hpp"

#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/menu/CreativeUiDrawList.hpp"

namespace iggy3d {
namespace {

std::string_view stableCreativeUiStatus(std::string_view status) noexcept {
  if (status == "product_creative_ui_draw_list_ready") {
    return "product_creative_ui_draw_list_ready";
  }
  if (status == "product_creative_ui_model_missing") {
    return "product_creative_ui_model_missing";
  }
  return "product_creative_ui_status_unknown";
}

void mirrorDrawListReceipt(ProductCreativeUiProjectionReceipt& receipt,
                           const ProductUiDrawList& drawList) noexcept {
  receipt.ready = drawList.ready;
  receipt.partial = drawList.partial;
  receipt.status = stableCreativeUiStatus(drawList.status);
  receipt.reasonCode = stableCreativeUiStatus(drawList.reasonCode);
  receipt.virtualWidth = drawList.virtualWidth;
  receipt.virtualHeight = drawList.virtualHeight;
  receipt.theme = drawList.theme;
  receipt.primitiveCount = drawList.primitiveCount;
  receipt.textCount = drawList.textCount;
  receipt.rectCount = drawList.rectCount;
  receipt.rowCount = drawList.rowCount;
  receipt.disabledRowCount = drawList.disabledRowCount;
  receipt.hitRegionCount = drawList.hitRegionCount;
}

void mirrorModelReceipt(ProductCreativeUiProjectionReceipt& receipt,
                        const creative::CreativeUiModel& model) noexcept {
  receipt.panelCount = model.panels.size();
  receipt.modelRowCount = model.rows.size();
}

ProductCreativeUiProjectionReceipt makeRequestedReceipt(
    const ProductCreativeUiProjectionRequest& request) noexcept {
  ProductCreativeUiProjectionReceipt receipt;
  receipt.requested = true;
  receipt.virtualWidth = request.virtualWidth;
  receipt.virtualHeight = request.virtualHeight;
  receipt.theme = request.theme;
  return receipt;
}

}  // namespace

ProductCreativeUiProjection buildProductCreativeUiProjection(
    const ProductCreativeUiProjectionRequest& request) {
  ProductCreativeUiProjection projection;
  projection.receipt = makeRequestedReceipt(request);

  creative::CreativeUiBuildReceipt facadeUiReceipt;
  const creative::CreativeUiModel* model = request.model;
  if (model != nullptr) {
    projection.receipt.usedModel = true;
  } else if (request.facade != nullptr) {
    facadeUiReceipt = request.facade->buildUiModel();
    model = &facadeUiReceipt.model;
    projection.receipt.usedFacade = true;
  }

  ProductCreativeUiDrawListRequest drawListRequest;
  drawListRequest.model = model;
  drawListRequest.virtualWidth = request.virtualWidth;
  drawListRequest.virtualHeight = request.virtualHeight;
  drawListRequest.theme = request.theme;
  projection.drawList = buildProductCreativeUiDrawList(drawListRequest);
  mirrorDrawListReceipt(projection.receipt, projection.drawList);

  if (model != nullptr) {
    mirrorModelReceipt(projection.receipt, *model);
  }

  return projection;
}

}  // namespace iggy3d
