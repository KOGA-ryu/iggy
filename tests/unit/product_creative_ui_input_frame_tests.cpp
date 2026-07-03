#include "app/iggy3d/window/CreativeUiInputFrame.hpp"

#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/creative/Ui.hpp"
#include "app/iggy3d/menu/CreativeUiDrawList.hpp"
#include "app/iggy3d/window/InputFrame.hpp"
#include "app/frontend/WorldSetupModel.hpp"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    return false;
  }
  return true;
}

iggy3d::RenderReceipt receiptFor(const iggy3d::ProductAppWindowState& window) {
  iggy3d::ProductAppOptions options;
  iggy3d::ProductWorldTemplate world;
  iggy3d::FrontendState frontend;
  iggy3d::FrontendSettings settings;
  iggy3d::ProductSaveBridgeResult saves;
  return iggy3d::buildProductAppReceipt(options,
                                        world,
                                        frontend,
                                        settings,
                                        window,
                                        saves);
}

bool expectReceiptField(const iggy3d::RenderReceipt& receipt,
                        std::string_view key,
                        std::string_view value,
                        std::string_view message) {
  return expect(iggy3d::hasReceiptField(receipt, key, value), message);
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

iggy3d::ProductCreativeUiInputFrameReceipt consumedActiveRowReceipt() {
  const iggy3d::ProductUiDrawList drawList = defaultCreativeDrawList();
  const iggy3d::UiHitRegion& activeHit = drawList.hitRegions.front();

  iggy3d::ProductCreativeUiInputFrameRequest request;
  request.creativeUiDrawList = &drawList;
  request.click = clickAt(activeHit.rect.x, activeHit.rect.y);
  return iggy3d::routeProductCreativeUiInputFrame(request);
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

bool defaultWindowReceiptFieldsAreNotRequested() {
  const iggy3d::ProductAppWindowState window;
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expectReceiptField(receipt,
                            "creative_ui_input_requested",
                            "false",
                            "default requested") &&
         expectReceiptField(receipt,
                            "creative_ui_input_click_present",
                            "false",
                            "default click present") &&
         expectReceiptField(receipt,
                            "creative_ui_input_draw_list_available",
                            "false",
                            "default draw list available") &&
         expectReceiptField(receipt,
                            "creative_ui_input_routed",
                            "false",
                            "default routed") &&
         expectReceiptField(receipt,
                            "creative_ui_input_hit",
                            "false",
                            "default hit") &&
         expectReceiptField(receipt,
                            "creative_ui_input_consumed",
                            "false",
                            "default consumed") &&
         expectReceiptField(receipt,
                            "creative_ui_input_enabled",
                            "false",
                            "default enabled") &&
         expectReceiptField(receipt,
                            "creative_ui_input_surface",
                            "none",
                            "default surface") &&
         expectReceiptField(receipt,
                            "creative_ui_input_kind",
                            "none",
                            "default kind") &&
         expectReceiptField(receipt,
                            "creative_ui_input_action",
                            "none",
                            "default action") &&
         expectReceiptField(receipt,
                            "creative_ui_input_layer_index",
                            "0",
                            "default layer index") &&
         expectReceiptField(receipt,
                            "creative_ui_input_region_index",
                            "0",
                            "default region index") &&
         expectReceiptField(receipt,
                            "creative_ui_input_semantic_id",
                            "none",
                            "default semantic id") &&
         expectReceiptField(receipt,
                            "creative_ui_input_status",
                            "creative_ui_input_not_requested",
                            "default status") &&
         expectReceiptField(receipt,
                            "creative_ui_input_reason_code",
                            "creative_ui_input_not_requested",
                            "default reason");
}

bool recorderCopiesNoClickReceipt() {
  const iggy3d::ProductUiDrawList drawList = defaultCreativeDrawList();
  iggy3d::ProductCreativeUiInputFrameRequest request;
  request.creativeUiDrawList = &drawList;
  const iggy3d::ProductCreativeUiInputFrameReceipt inputReceipt =
      iggy3d::routeProductCreativeUiInputFrame(request);

  iggy3d::ProductAppWindowState window;
  iggy3d::recordProductCreativeUiInputFrame(window, inputReceipt);
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expectReceiptField(receipt,
                            "creative_ui_input_requested",
                            "true",
                            "no click requested") &&
         expectReceiptField(receipt,
                            "creative_ui_input_click_present",
                            "false",
                            "no click absent") &&
         expectReceiptField(receipt,
                            "creative_ui_input_draw_list_available",
                            "true",
                            "no click draw list available") &&
         expectReceiptField(receipt,
                            "creative_ui_input_routed",
                            "false",
                            "no click not routed") &&
         expectReceiptField(receipt,
                            "creative_ui_input_surface",
                            "none",
                            "no click surface") &&
         expectReceiptField(receipt,
                            "creative_ui_input_kind",
                            "none",
                            "no click kind") &&
         expectReceiptField(receipt,
                            "creative_ui_input_action",
                            "none",
                            "no click action") &&
         expectReceiptField(receipt,
                            "creative_ui_input_semantic_id",
                            "none",
                            "no click semantic") &&
         expectReceiptField(receipt,
                            "creative_ui_input_status",
                            "product_creative_ui_input_no_click",
                            "no click status") &&
         expectReceiptField(receipt,
                            "creative_ui_input_reason_code",
                            "product_creative_ui_input_no_click",
                            "no click reason");
}

bool recorderCopiesConsumedCreativeRowReceipt() {
  const iggy3d::ProductCreativeUiInputFrameReceipt inputReceipt =
      consumedActiveRowReceipt();

  iggy3d::ProductAppWindowState window;
  iggy3d::recordProductCreativeUiInputFrame(window, inputReceipt);
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expectReceiptField(receipt,
                            "creative_ui_input_requested",
                            "true",
                            "consumed requested") &&
         expectReceiptField(receipt,
                            "creative_ui_input_click_present",
                            "true",
                            "consumed click present") &&
         expectReceiptField(receipt,
                            "creative_ui_input_draw_list_available",
                            "true",
                            "consumed draw available") &&
         expectReceiptField(receipt,
                            "creative_ui_input_routed",
                            "true",
                            "consumed routed") &&
         expectReceiptField(receipt,
                            "creative_ui_input_hit",
                            "true",
                            "consumed hit") &&
         expectReceiptField(receipt,
                            "creative_ui_input_consumed",
                            "true",
                            "consumed consumed") &&
         expectReceiptField(receipt,
                            "creative_ui_input_enabled",
                            "true",
                            "consumed enabled") &&
         expectReceiptField(receipt,
                            "creative_ui_input_surface",
                            "creative_overlay",
                            "consumed surface") &&
         expectReceiptField(receipt,
                            "creative_ui_input_kind",
                            "row",
                            "consumed kind") &&
         expectReceiptField(receipt,
                            "creative_ui_input_action",
                            "none",
                            "consumed action") &&
         expectReceiptField(receipt,
                            "creative_ui_input_layer_index",
                            "0",
                            "consumed layer index") &&
         expectReceiptField(receipt,
                            "creative_ui_input_region_index",
                            "0",
                            "consumed region index") &&
         expectReceiptField(receipt,
                            "creative_ui_input_semantic_id",
                            "creative.row.tools.active_tool",
                            "consumed semantic") &&
         expectReceiptField(receipt,
                            "creative_ui_input_status",
                            "product_creative_ui_input_consumed",
                            "consumed status");
}

bool recorderLeavesOtherReceiptFieldsUntouched() {
  iggy3d::ProductAppWindowState window;
  window.status = "window_status_before";
  window.productVulkanMenuUiReady = true;
  window.productVulkanMenuUiStatus = "vulkan_ui_before";
  window.productVulkanMenuUiSelectedAction = "resume";
  window.creativeUiProjectionRequested = true;
  window.creativeUiProjectionReady = true;
  window.creativeUiProjectionStatus = "projection_before";
  window.creativeUiProjectionHitRegionCount = 12;

  iggy3d::recordProductCreativeUiInputFrame(window, consumedActiveRowReceipt());

  return expect(window.status == "window_status_before",
                "window status unchanged") &&
         expect(window.productVulkanMenuUiReady,
                "vulkan ui ready unchanged") &&
         expect(window.productVulkanMenuUiStatus == "vulkan_ui_before",
                "vulkan ui status unchanged") &&
         expect(window.productVulkanMenuUiSelectedAction == "resume",
                "vulkan selected action unchanged") &&
         expect(window.creativeUiProjectionRequested,
                "projection requested unchanged") &&
         expect(window.creativeUiProjectionReady,
                "projection ready unchanged") &&
         expect(window.creativeUiProjectionStatus == "projection_before",
                "projection status unchanged") &&
         expect(window.creativeUiProjectionHitRegionCount == 12U,
                "projection hit count unchanged");
}

bool inputFrameNoClickNullDrawListRecordsNoClick() {
  iggy3d::FrontendState frontend;
  iggy3d::ProductSaveBridgeResult saves;
  iggy3d::ProductAppOptions options;
  iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::Input;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::WorldSetupDraft worldSetupDraft;
  iggy3d::ProductAppWindowState window;
  iggy3d::FrontendSettings settings;
  iggy3d::ProductWindowInputFrameState inputFrame;
  bool closeRequested = false;

  iggy3d::processProductWindowInputFrame(iggy3d::ProductWindowInputFrameContext{
      frontend,
      saves,
      options,
      settingsTab,
      activeSession,
      worldSetupDraft,
      window,
      settings,
      inputFrame,
      closeRequested,
      nullptr,
      nullptr,
      nullptr,
  });

  return expect(window.creativeUiInputRequested,
                "input frame requested") &&
         expect(!window.creativeUiInputClickPresent,
                "input frame no click") &&
         expect(!window.creativeUiInputDrawListAvailable,
                "input frame no draw list") &&
         expect(!window.creativeUiInputRouted,
                "input frame not routed") &&
         expect(window.creativeUiInputStatus ==
                    "product_creative_ui_input_no_click",
                "input frame status") &&
         expect(window.creativeUiInputReasonCode ==
                    "product_creative_ui_input_no_click",
                "input frame reason");
}

}  // namespace

int main() {
  const bool ok = noClickDoesNotRouteAndPreservesDrawListAvailability() &&
                  clickedNullDrawListFailsClosed() &&
                  clickedNotReadyDrawListRoutesAndReportsNotReady() &&
                  clickedReadyCreativeRowConsumes() &&
                  clickedOutsideReadyDrawListMisses() &&
                  disabledCreativeRowReportsHitDisabled() &&
                  receiptCopiesKnownRegionIndex() &&
                  defaultWindowReceiptFieldsAreNotRequested() &&
                  recorderCopiesNoClickReceipt() &&
                  recorderCopiesConsumedCreativeRowReceipt() &&
                  recorderLeavesOtherReceiptFieldsUntouched() &&
                  inputFrameNoClickNullDrawListRecordsNoClick();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
