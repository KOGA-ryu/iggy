#include "app/iggy3d/window/CreativeUiInputFrame.hpp"

#include "app/iggy3d/creative/Ui.hpp"
#include "app/iggy3d/menu/CreativeUiDrawList.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    return false;
  }
  return true;
}

iggy3d::MouseClick clickAt(float x, float y) {
  iggy3d::MouseClick click;
  click.clicked = true;
  click.x = x;
  click.y = y;
  return click;
}

iggy3d::creative::CreativeUiModel defaultCreativeModel() {
  return iggy3d::creative::buildCreativeUiModel(
             iggy3d::creative::makeDefaultCreativeUiBuildRequest())
      .model;
}

iggy3d::ProductUiDrawList defaultCreativeDrawList() {
  const iggy3d::creative::CreativeUiModel model = defaultCreativeModel();
  iggy3d::ProductCreativeUiDrawListRequest request;
  request.model = &model;
  return iggy3d::buildProductCreativeUiDrawList(request);
}

iggy3d::ProductUiDrawList disabledActiveRowCreativeDrawList() {
  iggy3d::creative::CreativeUiModel model = defaultCreativeModel();
  for (iggy3d::creative::CreativeUiRow& row : model.rows) {
    if (row.id == "active_tool") {
      row.flags &= ~iggy3d::creative::kCreativeUiRowFlagEnabled;
      break;
    }
  }
  iggy3d::ProductCreativeUiDrawListRequest request;
  request.model = &model;
  return iggy3d::buildProductCreativeUiDrawList(request);
}

bool noClickDoesNotRouteAndPreservesDrawListAvailability() {
  const iggy3d::ProductUiDrawList drawList = defaultCreativeDrawList();
  const iggy3d::ProductCreativeUiInputFrameReceipt nullReceipt =
      iggy3d::routeProductCreativeUiInputFrame({});

  iggy3d::ProductCreativeUiInputFrameRequest request;
  request.creativeUiDrawList = &drawList;
  const iggy3d::ProductCreativeUiInputFrameReceipt availableReceipt =
      iggy3d::routeProductCreativeUiInputFrame(request);

  return expect(nullReceipt.requested, "null no-click requested") &&
         expect(!nullReceipt.clickPresent, "null click absent") &&
         expect(!nullReceipt.drawListAvailable, "null draw list unavailable") &&
         expect(!nullReceipt.routed, "null not routed") &&
         expect(!nullReceipt.hit, "null no hit") &&
         expect(!nullReceipt.consumed, "null not consumed") &&
         expect(nullReceipt.status == "product_creative_ui_input_no_click",
                "null no-click status") &&
         expect(availableReceipt.drawListAvailable,
                "available draw list reflected") &&
         expect(!availableReceipt.routed, "available no-click not routed") &&
         expect(availableReceipt.reasonCode ==
                    "product_creative_ui_input_no_click",
                "available no-click reason");
}

bool clickedNullDrawListFailsClosed() {
  iggy3d::ProductCreativeUiInputFrameRequest request;
  request.click = clickAt(10.0F, 10.0F);
  const iggy3d::ProductCreativeUiInputFrameReceipt receipt =
      iggy3d::routeProductCreativeUiInputFrame(request);

  return expect(receipt.requested, "null draw requested") &&
         expect(receipt.clickPresent, "null draw click present") &&
         expect(!receipt.drawListAvailable, "null draw unavailable") &&
         expect(!receipt.routed, "null draw not routed") &&
         expect(!receipt.hit, "null draw no hit") &&
         expect(!receipt.consumed, "null draw not consumed") &&
         expect(receipt.status ==
                    "product_creative_ui_input_draw_list_missing",
                "null draw status");
}

bool clickedNotReadyDrawListRoutesAndReportsNotReady() {
  iggy3d::ProductUiDrawList drawList;
  drawList.ready = false;
  drawList.hitRegions.push_back(
      iggy3d::UiHitRegion{"creative.row.disabled",
                           {0.0F, 0.0F, 100.0F, 100.0F},
                           iggy3d::UiHitKind::Row,
                           iggy3d::FrontendAction::None,
                           true});
  drawList.hitRegionCount = drawList.hitRegions.size();

  iggy3d::ProductCreativeUiInputFrameRequest request;
  request.creativeUiDrawList = &drawList;
  request.click = clickAt(10.0F, 10.0F);
  const iggy3d::ProductCreativeUiInputFrameReceipt receipt =
      iggy3d::routeProductCreativeUiInputFrame(request);

  return expect(receipt.clickPresent, "not ready click present") &&
         expect(receipt.drawListAvailable, "not ready draw available") &&
         expect(receipt.routed, "not ready routes through router") &&
         expect(!receipt.hit, "not ready no hit") &&
         expect(!receipt.consumed, "not ready not consumed") &&
         expect(receipt.status == "product_creative_ui_input_not_ready",
                "not ready status");
}

bool clickedReadyCreativeRowConsumes() {
  const iggy3d::ProductUiDrawList drawList = defaultCreativeDrawList();
  const iggy3d::UiHitRegion& activeHit = drawList.hitRegions.front();

  iggy3d::ProductCreativeUiInputFrameRequest request;
  request.creativeUiDrawList = &drawList;
  request.click = clickAt(activeHit.rect.x, activeHit.rect.y);
  const iggy3d::ProductCreativeUiInputFrameReceipt receipt =
      iggy3d::routeProductCreativeUiInputFrame(request);

  return expect(receipt.requested, "ready requested") &&
         expect(receipt.clickPresent, "ready click present") &&
         expect(receipt.drawListAvailable, "ready draw available") &&
         expect(receipt.routed, "ready routed") &&
         expect(receipt.hit, "ready hit") &&
         expect(receipt.consumed, "ready consumed") &&
         expect(receipt.enabled, "ready enabled") &&
         expect(receipt.surface ==
                    iggy3d::ProductUiHitSurface::CreativeOverlay,
                "ready creative surface") &&
         expect(receipt.kind == iggy3d::UiHitKind::Row, "ready row kind") &&
         expect(receipt.action == iggy3d::FrontendAction::None,
                "ready action none") &&
         expect(receipt.regionIndex == 0U, "ready region index") &&
         expect(receipt.semanticId == activeHit.semanticId,
                "ready semantic copied") &&
         expect(receipt.status == "product_creative_ui_input_consumed",
                "ready consumed status");
}

bool clickedOutsideReadyDrawListMisses() {
  const iggy3d::ProductUiDrawList drawList = defaultCreativeDrawList();

  iggy3d::ProductCreativeUiInputFrameRequest request;
  request.creativeUiDrawList = &drawList;
  request.click = clickAt(1000.0F, 1000.0F);
  const iggy3d::ProductCreativeUiInputFrameReceipt receipt =
      iggy3d::routeProductCreativeUiInputFrame(request);

  return expect(receipt.routed, "miss routed") &&
         expect(!receipt.hit, "miss no hit") &&
         expect(!receipt.consumed, "miss not consumed") &&
         expect(receipt.status == "product_creative_ui_input_miss",
                "miss status");
}

bool disabledCreativeRowReportsHitDisabled() {
  const iggy3d::ProductUiDrawList drawList = disabledActiveRowCreativeDrawList();
  const iggy3d::UiHitRegion& activeHit = drawList.hitRegions.front();

  iggy3d::ProductCreativeUiInputFrameRequest request;
  request.creativeUiDrawList = &drawList;
  request.click = clickAt(activeHit.rect.x, activeHit.rect.y);
  const iggy3d::ProductCreativeUiInputFrameReceipt receipt =
      iggy3d::routeProductCreativeUiInputFrame(request);

  return expect(receipt.routed, "disabled routed") &&
         expect(receipt.hit, "disabled hit") &&
         expect(!receipt.consumed, "disabled not consumed") &&
         expect(!receipt.enabled, "disabled enabled false") &&
         expect(receipt.regionIndex == 0U, "disabled region index") &&
         expect(receipt.semanticId == activeHit.semanticId,
                "disabled semantic") &&
         expect(receipt.status == "product_creative_ui_input_hit_disabled",
                "disabled status");
}

bool receiptCopiesKnownRegionIndex() {
  const iggy3d::ProductUiDrawList drawList = defaultCreativeDrawList();
  const iggy3d::UiHitRegion& statusHit = drawList.hitRegions[1];

  iggy3d::ProductCreativeUiInputFrameRequest request;
  request.creativeUiDrawList = &drawList;
  request.click = clickAt(statusHit.rect.x, statusHit.rect.y);
  const iggy3d::ProductCreativeUiInputFrameReceipt receipt =
      iggy3d::routeProductCreativeUiInputFrame(request);

  return expect(receipt.hit, "known row hit") &&
         expect(receipt.regionIndex == 1U, "known row index copied") &&
         expect(receipt.semanticId == "creative.row.status.creative_status",
                "known row semantic copied");
}

}  // namespace

int main() {
  const bool ok = noClickDoesNotRouteAndPreservesDrawListAvailability() &&
                  clickedNullDrawListFailsClosed() &&
                  clickedNotReadyDrawListRoutesAndReportsNotReady() &&
                  clickedReadyCreativeRowConsumes() &&
                  clickedOutsideReadyDrawListMisses() &&
                  disabledCreativeRowReportsHitDisabled() &&
                  receiptCopiesKnownRegionIndex();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
