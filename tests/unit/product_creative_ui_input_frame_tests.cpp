#include "app/iggy3d/creative/bridge/UiInputFrame.hpp"

#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/ui/Ui.hpp"
#include "app/iggy3d/creative/ui/UiDrawList.hpp"
#include "app/iggy3d/creative/bridge/InputFrame.hpp"
#include "app/iggy3d/creative/bridge/UiCommandFrame.hpp"
#include "app/iggy3d/creative/bridge/UiWindowFrame.hpp"
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

void markCreativeDocumentWindow(iggy3d::ProductAppWindowState& window) {
  window.interactionMode = iggy3d::ProductInteractionMode::Creative;
  window.activeCreativeSaveId = "creative_save";
  window.activeCreativeWorldId = "world_001";
  window.activeCreativeDocumentId = 42U;
}

void assignValidDocumentId(iggy3d::creative::Facade& facade) {
  static_cast<void>(facade.documentForPersistence().assignId(42U));
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
    if (row.id == "tool_select") {
      row.flags &= ~iggy3d::creative::kCreativeUiRowFlagEnabled;
      break;
    }
  }
  iggy3d::ProductCreativeUiDrawListRequest request;
  request.model = &model;
  return iggy3d::buildProductCreativeUiDrawList(request);
}

const iggy3d::UiHitRegion* findHitRegion(
    const iggy3d::ProductUiDrawList& drawList,
    std::string_view semanticId) {
  for (const iggy3d::UiHitRegion& hit : drawList.hitRegions) {
    if (hit.semanticId == semanticId) {
      return &hit;
    }
  }
  return nullptr;
}

const iggy3d::ProductUiPrimitive* findPrimitive(
    const iggy3d::ProductUiDrawList& drawList,
    std::string_view semanticId) {
  for (const iggy3d::ProductUiPrimitive& primitive : drawList.primitives) {
    if (primitive.semanticId == semanticId) {
      return &primitive;
    }
  }
  return nullptr;
}

iggy3d::ProductCreativeUiInputFrameReceipt consumedActiveRowReceipt() {
  const iggy3d::ProductUiDrawList drawList = defaultCreativeDrawList();
  const iggy3d::UiHitRegion* activeHit =
      findHitRegion(drawList, "creative.row.tools.tool_select");

  iggy3d::ProductCreativeUiInputFrameRequest request;
  request.creativeUiDrawList = &drawList;
  if (activeHit != nullptr) {
    request.click = clickAt(activeHit->rect.x, activeHit->rect.y);
  }
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

bool renderableOverlayInputConsumesReadyCreativeRow() {
  const iggy3d::ProductUiDrawList drawList = defaultCreativeDrawList();
  const iggy3d::UiHitRegion& activeHit = drawList.hitRegions.front();
  const iggy3d::ProductCreativeUiOverlayInputAvailability availability =
      iggy3d::resolveProductCreativeUiOverlayInputAvailability(
          iggy3d::ProductCreativeUiOverlayInputAvailabilityRequest{
              &drawList,
              true,
              true,
              1280,
              720,
              true});
  const iggy3d::ProductUiDrawList* inputDrawList =
      availability.inputAvailable ? &drawList : nullptr;

  iggy3d::ProductCreativeUiInputFrameRequest request;
  request.creativeUiDrawList = inputDrawList;
  request.click = clickAt(activeHit.rect.x, activeHit.rect.y);
  const iggy3d::ProductCreativeUiInputFrameReceipt receipt =
      iggy3d::routeProductCreativeUiInputFrame(request);

  return expect(availability.inputAvailable, "renderable input available") &&
         expect(receipt.drawListAvailable, "renderable draw available") &&
         expect(receipt.routed, "renderable routed") &&
         expect(receipt.consumed, "renderable consumed") &&
         expect(receipt.semanticId == "creative.row.tools.tool_select",
                "renderable semantic");
}

bool unrenderableOverlayInputDoesNotSuppressToolClick() {
  const iggy3d::ProductUiDrawList drawList = defaultCreativeDrawList();
  const iggy3d::UiHitRegion& activeHit = drawList.hitRegions.front();
  const iggy3d::MouseClick click =
      clickAt(activeHit.rect.x, activeHit.rect.y);
  const iggy3d::ProductCreativeUiOverlayInputAvailability availability =
      iggy3d::resolveProductCreativeUiOverlayInputAvailability(
          iggy3d::ProductCreativeUiOverlayInputAvailabilityRequest{
              &drawList,
              false,
              true,
              1280,
              720,
              true});
  const iggy3d::ProductUiDrawList* inputDrawList =
      availability.inputAvailable ? &drawList : nullptr;

  iggy3d::ProductCreativeUiInputFrameRequest inputRequest;
  inputRequest.creativeUiDrawList = inputDrawList;
  inputRequest.click = click;
  const iggy3d::ProductCreativeUiInputFrameReceipt inputReceipt =
      iggy3d::routeProductCreativeUiInputFrame(inputRequest);
  const iggy3d::ProductCreativeUiDownstreamClickReceipt downstream =
      iggy3d::routeProductCreativeUiDownstreamClick(
          iggy3d::ProductCreativeUiDownstreamClickRequest{
              click,
              inputReceipt.consumed,
              false,
          });

  iggy3d::ProductAppWindowState window;
  markCreativeDocumentWindow(window);
  iggy3d::creative::CreativeAppState app;
  [[maybe_unused]] iggy3d::creative::Facade& facade = app.facade;
  facade.reset();
  static_cast<void>(facade.setActiveTool(iggy3d::creative::Tool::Measure));
  const iggy3d::ProductCreativeInputFrameReceipt toolReceipt =
      iggy3d::processProductCreativeInputActions(
          iggy3d::ProductCreativeInputActionsRequest{
              &window,
              &app,
              nullptr,
              {},
              downstream.downstreamClick,
              {},
          });

  return expect(!availability.inputAvailable,
                "unrenderable input unavailable") &&
         expect(inputReceipt.clickPresent, "unrenderable click present") &&
         expect(!inputReceipt.drawListAvailable,
                "unrenderable draw list withheld") &&
         expect(inputReceipt.status ==
                    "product_creative_ui_input_draw_list_missing",
                "unrenderable route fails closed") &&
         expect(!inputReceipt.consumed, "unrenderable not consumed") &&
         expect(!downstream.suppressed,
                "unrenderable downstream not suppressed") &&
         expect(downstream.downstreamClick.clicked,
                "unrenderable downstream click kept") &&
         expect(toolReceipt.pointerDispatched,
                "unrenderable tool pointer dispatched") &&
         expect(facade.measurementState().active,
                "unrenderable measurement active");
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
  const iggy3d::UiHitRegion& statusHit = drawList.hitRegions[7];

  iggy3d::ProductCreativeUiInputFrameRequest request;
  request.creativeUiDrawList = &drawList;
  request.click = clickAt(statusHit.rect.x, statusHit.rect.y);
  const iggy3d::ProductCreativeUiInputFrameReceipt receipt =
      iggy3d::routeProductCreativeUiInputFrame(request);

  return expect(receipt.hit, "known row hit") &&
         expect(receipt.regionIndex == 7U, "known row index copied") &&
         expect(receipt.semanticId == "creative.row.status.creative_status",
                "known row semantic copied");
}

bool downstreamNoClickDoesNotSuppress() {
  const iggy3d::ProductCreativeUiDownstreamClickReceipt receipt =
      iggy3d::routeProductCreativeUiDownstreamClick({});

  return expect(receipt.requested, "downstream no-click requested") &&
         expect(!receipt.clickPresent, "downstream no-click absent") &&
         expect(!receipt.creativeUiConsumed,
                "downstream no-click creative false") &&
         expect(!receipt.higherPriorityUiConsumed,
                "downstream no-click priority false") &&
         expect(!receipt.suppressed, "downstream no-click not suppressed") &&
         expect(!receipt.downstreamClick.clicked,
                "downstream no-click remains non-click") &&
         expect(receipt.status ==
                    "product_creative_ui_downstream_click_no_click",
                "downstream no-click status");
}

bool downstreamSuppressesCreativeConsumedClick() {
  iggy3d::ProductCreativeUiDownstreamClickRequest request;
  request.click = clickAt(11.0F, 12.0F);
  request.creativeUiConsumed = true;
  const iggy3d::ProductCreativeUiDownstreamClickReceipt receipt =
      iggy3d::routeProductCreativeUiDownstreamClick(request);

  return expect(receipt.clickPresent, "suppressed click present") &&
         expect(receipt.creativeUiConsumed, "suppressed creative consumed") &&
         expect(!receipt.higherPriorityUiConsumed,
                "suppressed no higher priority") &&
         expect(receipt.suppressed, "suppressed true") &&
         expect(!receipt.downstreamClick.clicked,
                "suppressed downstream non-click") &&
         expect(receipt.downstreamClick.x == 11.0F,
                "suppressed downstream x preserved") &&
         expect(receipt.downstreamClick.y == 12.0F,
                "suppressed downstream y preserved") &&
         expect(receipt.status ==
                    "product_creative_ui_downstream_click_suppressed",
                "suppressed status");
}

bool downstreamHigherPrioritySuppressesClick() {
  iggy3d::ProductCreativeUiDownstreamClickRequest request;
  request.click = clickAt(21.0F, 22.0F);
  request.creativeUiConsumed = true;
  request.higherPriorityUiConsumed = true;
  const iggy3d::ProductCreativeUiDownstreamClickReceipt receipt =
      iggy3d::routeProductCreativeUiDownstreamClick(request);

  return expect(receipt.clickPresent, "priority click present") &&
         expect(receipt.creativeUiConsumed, "priority creative consumed") &&
         expect(receipt.higherPriorityUiConsumed,
                "priority higher consumed") &&
         expect(receipt.suppressed, "priority suppressed") &&
         expect(!receipt.downstreamClick.clicked,
                "priority downstream suppressed") &&
         expect(receipt.downstreamClick.x == 21.0F,
                "priority x preserved") &&
         expect(receipt.downstreamClick.y == 22.0F,
                "priority y preserved") &&
         expect(receipt.status ==
                    "product_creative_ui_downstream_click_higher_priority",
                "priority status");
}

bool downstreamPassthroughWhenCreativeDoesNotConsume() {
  iggy3d::ProductCreativeUiDownstreamClickRequest request;
  request.click = clickAt(31.0F, 32.0F);
  const iggy3d::ProductCreativeUiDownstreamClickReceipt receipt =
      iggy3d::routeProductCreativeUiDownstreamClick(request);

  return expect(receipt.clickPresent, "passthrough click present") &&
         expect(!receipt.creativeUiConsumed,
                "passthrough creative false") &&
         expect(!receipt.suppressed, "passthrough not suppressed") &&
         expect(receipt.downstreamClick.clicked,
                "passthrough downstream clicked") &&
         expect(receipt.downstreamClick.x == 31.0F,
                "passthrough x preserved") &&
         expect(receipt.downstreamClick.y == 32.0F,
                "passthrough y preserved") &&
         expect(receipt.status ==
                    "product_creative_ui_downstream_click_passthrough",
                "passthrough status");
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
                            "default reason") &&
         expectReceiptField(receipt,
                            "creative_ui_input_downstream_click_requested",
                            "false",
                            "default downstream requested") &&
         expectReceiptField(receipt,
                            "creative_ui_input_downstream_click_present",
                            "false",
                            "default downstream present") &&
         expectReceiptField(receipt,
                            "creative_ui_input_downstream_click_higher_priority",
                            "false",
                            "default downstream priority") &&
         expectReceiptField(receipt,
                            "creative_ui_input_downstream_click_suppressed",
                            "false",
                            "default downstream suppressed") &&
         expectReceiptField(receipt,
                            "creative_ui_input_downstream_click_status",
                            "creative_ui_input_downstream_click_not_requested",
                            "default downstream status") &&
         expectReceiptField(receipt,
                            "creative_ui_input_downstream_click_reason_code",
                            "creative_ui_input_downstream_click_not_requested",
                            "default downstream reason") &&
         expectReceiptField(receipt,
                            "creative_document_revision_observed",
                            "false",
                            "default revision not observed") &&
         expectReceiptField(receipt,
                            "creative_document_changed_this_frame",
                            "false",
                            "default document unchanged") &&
         expectReceiptField(receipt,
                            "creative_baked_room_stale",
                            "false",
                            "default baked room not stale") &&
         expectReceiptField(receipt,
                            "creative_baked_room_stale_status",
                            "creative_baked_room_not_observed",
                            "default baked room stale status") &&
         expectReceiptField(receipt,
                            "creative_baked_room_auto_refresh_requested",
                            "false",
                            "default auto refresh not requested") &&
         expectReceiptField(receipt,
                            "creative_baked_room_auto_refresh_accepted",
                            "false",
                            "default auto refresh not accepted") &&
         expectReceiptField(receipt,
                            "creative_baked_room_auto_refresh_cleared_active_room",
                            "false",
                            "default auto refresh not cleared") &&
         expectReceiptField(receipt,
                            "creative_baked_room_auto_refresh_status",
                            "product_creative_baked_room_not_requested",
                            "default auto refresh status");
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
                            "creative.row.tools.tool_select",
                            "consumed semantic") &&
         expectReceiptField(receipt,
                            "creative_ui_input_status",
                            "product_creative_ui_input_consumed",
                            "consumed status");
}

bool recorderPreservesStickyClickAndCommandAcrossNoClickFrame() {
  iggy3d::creative::CreativeAppState app;
  [[maybe_unused]] iggy3d::creative::Facade& facade = app.facade;
  facade.reset();
  const iggy3d::creative::CreativeUiBuildReceipt ui = facade.buildUiModel();
  iggy3d::ProductCreativeUiDrawListRequest drawRequest;
  drawRequest.model = &ui.model;
  const iggy3d::ProductUiDrawList drawList =
      iggy3d::buildProductCreativeUiDrawList(drawRequest);
  const iggy3d::UiHitRegion* createHit =
      findHitRegion(drawList, "creative.row.create.create_room");

  iggy3d::ProductAppWindowState window;
  iggy3d::ProductCreativeUiInputFrameRequest clickRequest;
  clickRequest.creativeUiDrawList = &drawList;
  if (createHit != nullptr) {
    clickRequest.click = clickAt(createHit->rect.x, createHit->rect.y);
  }
  const iggy3d::ProductCreativeUiInputFrameReceipt clickReceipt =
      iggy3d::routeProductCreativeUiInputFrame(clickRequest);
  iggy3d::recordProductCreativeUiInputFrame(window, clickReceipt);
  const iggy3d::ProductCreativeUiCommandFrameReceipt commandReceipt =
      iggy3d::routeProductCreativeUiCommandFrame(
          iggy3d::ProductCreativeUiCommandFrameRequest{&app, clickReceipt});
  iggy3d::recordProductCreativeUiCommandFrame(window, commandReceipt);
  const std::uint64_t createdObjectId =
      window.creativeUiLastCommandCreateObjectId;

  iggy3d::ProductCreativeUiInputFrameRequest noClickRequest;
  noClickRequest.creativeUiDrawList = &drawList;
  const iggy3d::ProductCreativeUiInputFrameReceipt noClickReceipt =
      iggy3d::routeProductCreativeUiInputFrame(noClickRequest);
  iggy3d::recordProductCreativeUiInputFrame(window, noClickReceipt);
  const iggy3d::ProductCreativeUiCommandFrameReceipt noClickCommandReceipt =
      iggy3d::routeProductCreativeUiCommandFrame(
          iggy3d::ProductCreativeUiCommandFrameRequest{&app,
                                                       noClickReceipt});
  iggy3d::recordProductCreativeUiCommandFrame(window, noClickCommandReceipt);
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expect(createHit != nullptr, "sticky create hit exists") &&
         expect(clickReceipt.clickPresent, "sticky click present") &&
         expect(commandReceipt.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::CreateObject,
                "sticky command create room") &&
         expect(window.creativeUiInputStatus ==
                    "product_creative_ui_input_no_click",
                "sticky per-frame no-click status") &&
         expect(window.creativeUiCommandKind == "none",
                "sticky per-frame command none") &&
         expect(window.creativeUiLastClickSeen,
                "sticky click seen retained") &&
         expect(window.creativeUiLastClickX != "none",
                "sticky click x retained") &&
         expect(window.creativeUiLastClickY != "none",
                "sticky click y retained") &&
         expect(window.creativeUiLastInputHit,
                "sticky input hit retained") &&
         expect(window.creativeUiLastInputConsumed,
                "sticky input consumed retained") &&
         expect(window.creativeUiLastInputStatus ==
                    "product_creative_ui_input_consumed",
                "sticky input status retained") &&
         expect(window.creativeUiLastInputSemanticId ==
                    "creative.row.create.create_room",
                "sticky input semantic retained") &&
         expect(window.creativeUiLastCommandKind == "create_object",
                "sticky command kind retained") &&
         expect(window.creativeUiLastCommandStatus ==
                    "product_creative_ui_command_applied",
                "sticky command status retained") &&
         expect(window.creativeUiLastCommandCreateRequested,
                "sticky create requested retained") &&
         expect(window.creativeUiLastCommandCreateAccepted,
                "sticky create accepted retained") &&
         expect(window.creativeUiLastCommandCreateChanged,
                "sticky create changed retained") &&
         expect(createdObjectId != 0U, "sticky create object id retained") &&
         expectReceiptField(receipt,
                            "creative_ui_last_click_seen",
                            "true",
                            "sticky receipt click seen") &&
         expectReceiptField(receipt,
                            "creative_ui_last_input_semantic_id",
                            "creative.row.create.create_room",
                            "sticky receipt semantic") &&
         expectReceiptField(receipt,
                            "creative_ui_last_command_kind",
                            "create_object",
                            "sticky receipt command kind") &&
         expectReceiptField(receipt,
                            "creative_ui_last_command_create_requested",
                            "true",
                            "sticky receipt create requested") &&
         expectReceiptField(receipt,
                            "creative_ui_last_command_create_object_id",
                            std::to_string(createdObjectId),
                            "sticky receipt create object id");
}

bool recorderCopiesSuppressedDownstreamClickReceipt() {
  iggy3d::ProductCreativeUiDownstreamClickRequest request;
  request.click = clickAt(41.0F, 42.0F);
  request.creativeUiConsumed = true;
  const iggy3d::ProductCreativeUiDownstreamClickReceipt clickReceipt =
      iggy3d::routeProductCreativeUiDownstreamClick(request);

  iggy3d::ProductAppWindowState window;
  iggy3d::recordProductCreativeUiDownstreamClick(window, clickReceipt);
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expectReceiptField(receipt,
                            "creative_ui_input_downstream_click_requested",
                            "true",
                            "suppressed receipt requested") &&
         expectReceiptField(receipt,
                            "creative_ui_input_downstream_click_present",
                            "true",
                            "suppressed receipt present") &&
         expectReceiptField(receipt,
                            "creative_ui_input_downstream_click_higher_priority",
                            "false",
                            "suppressed receipt priority") &&
         expectReceiptField(receipt,
                            "creative_ui_input_downstream_click_suppressed",
                            "true",
                            "suppressed receipt suppressed") &&
         expectReceiptField(receipt,
                            "creative_ui_input_downstream_click_status",
                            "product_creative_ui_downstream_click_suppressed",
                            "suppressed receipt status") &&
         expectReceiptField(receipt,
                            "creative_ui_input_downstream_click_reason_code",
                            "product_creative_ui_downstream_click_suppressed",
                            "suppressed receipt reason");
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
  window.creativeUiInputRequested = true;
  window.creativeUiInputConsumed = true;
  window.creativeUiInputStatus = "route_before";

  iggy3d::ProductCreativeUiDownstreamClickRequest request;
  request.click = clickAt(51.0F, 52.0F);
  request.creativeUiConsumed = true;
  iggy3d::recordProductCreativeUiDownstreamClick(
      window, iggy3d::routeProductCreativeUiDownstreamClick(request));

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
                "projection hit count unchanged") &&
         expect(window.creativeUiInputRequested,
                "route requested unchanged") &&
         expect(window.creativeUiInputConsumed,
                "route consumed unchanged") &&
         expect(window.creativeUiInputStatus == "route_before",
                "route status unchanged");
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
  iggy3d::creative::CreativeAppState app;
  [[maybe_unused]] iggy3d::creative::Facade& facade = app.facade;
  facade.reset();
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
      &app,
      nullptr,
      {},
      {},
      0,
      iggy3d::creative::CreativeViewportPickDepthMode::FixedZ,
      {},
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
                "input frame reason") &&
         expect(window.creativeUiInputDownstreamClickRequested,
                "input frame downstream requested") &&
         expect(!window.creativeUiInputDownstreamClickPresent,
                "input frame downstream no click") &&
         expect(!window.creativeUiInputDownstreamClickSuppressed,
                "input frame downstream not suppressed") &&
         expect(window.creativeUiInputDownstreamClickStatus ==
                    "product_creative_ui_downstream_click_no_click",
                "input frame downstream status") &&
         expect(window.creativeUiCommandRequested,
                "input frame command requested") &&
         expect(window.creativeUiCommandFacadeAvailable,
                "input frame command facade available") &&
         expect(!window.creativeUiCommandInputConsumed,
                "input frame command input not consumed") &&
         expect(!window.creativeUiCommandAccepted,
                "input frame command not accepted") &&
         expect(!window.creativeUiCommandChanged,
                "input frame command unchanged") &&
         expect(window.creativeUiCommandKind == "none",
                "input frame command kind none") &&
         expect(window.creativeUiCommandToolBefore == "Select",
                "input frame command tool before") &&
         expect(window.creativeUiCommandToolAfter == "Select",
                "input frame command tool after") &&
         expect(window.creativeUiCommandSemanticId == "none",
                "input frame command semantic none") &&
         expect(window.creativeUiCommandStatus ==
                    "product_creative_ui_command_not_consumed",
                "input frame command status") &&
         expect(facade.toolState().activeTool ==
                    iggy3d::creative::Tool::Select,
                "input frame facade tool unchanged");
}

bool inputFrameInjectedClickOnToolSelectRowSetsToolAndSuppressesClick() {
  iggy3d::FrontendState frontend;
  iggy3d::enterFrontendGameplay(frontend, iggy3d::FrontendAction::NewWorld);
  iggy3d::ProductSaveBridgeResult saves;
  iggy3d::ProductAppOptions options;
  iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::Input;
  std::optional<iggy3d::Session> activeSession;
  activeSession.emplace();
  iggy3d::WorldSetupDraft worldSetupDraft;
  iggy3d::ProductAppWindowState window;
  window.gameplayActive = true;
  markCreativeDocumentWindow(window);
  iggy3d::FrontendSettings settings;
  iggy3d::ProductWindowInputFrameState inputFrame;
  iggy3d::creative::CreativeAppState app;
  [[maybe_unused]] iggy3d::creative::Facade& facade = app.facade;
  facade.reset();
  assignValidDocumentId(facade);
  bool closeRequested = false;

  const iggy3d::ProductUiDrawList drawList = defaultCreativeDrawList();
  const iggy3d::UiHitRegion& activeHit = drawList.hitRegions.front();
  iggy3d::ProductWindowInputClickOverride clickOverride;
  clickOverride.enabled = true;
  clickOverride.click = clickAt(activeHit.rect.x, activeHit.rect.y);

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
      &app,
      &drawList,
      {},
      {},
      0,
      iggy3d::creative::CreativeViewportPickDepthMode::FixedZ,
      clickOverride,
  });

  return expect(window.creativeUiInputConsumed,
                "injected row click consumed") &&
         expect(window.creativeUiInputSemanticId ==
                    "creative.row.tools.tool_select",
                "injected row semantic") &&
         expect(window.creativeUiCommandRequested,
                "injected command requested") &&
         expect(window.creativeUiCommandAccepted,
                "injected command accepted") &&
         expect(!window.creativeUiCommandChanged,
                "injected command unchanged") &&
         expect(window.creativeUiCommandKind == "set_active_tool",
                "injected command kind") &&
         expect(window.creativeUiCommandToolBefore == "Select",
                "injected command tool before") &&
         expect(window.creativeUiCommandToolAfter == "Select",
                "injected command tool after") &&
         expect(window.creativeUiCommandStatus ==
                    "product_creative_ui_command_no_change",
                "injected command status") &&
         expect(facade.toolState().activeTool ==
                    iggy3d::creative::Tool::Select,
                "injected facade tool select") &&
         expect(facade.state().tool == iggy3d::creative::Tool::Select,
                "injected old state tool select") &&
         expect(window.creativeUiInputDownstreamClickSuppressed,
                "injected downstream suppressed") &&
         expect(window.creativeViewportPickClickSuppressed,
                "injected viewport click suppressed") &&
         expect(!window.creativeViewportPickPicked,
                "injected viewport did not pick") &&
         expect(window.creativeViewportPickStatus ==
                    "product_creative_viewport_pick_no_click",
                "injected viewport no-click status") &&
         expect(facade.selectionState().selectedTarget.value ==
                    iggy3d::creative::kInvalidId,
                "injected no pointer selection") &&
         expect(!facade.measurementState().active,
                "injected no measurement dispatch") &&
         expect(window.creativeDocumentRevisionObserved,
                "injected revision observed") &&
         expect(!window.creativeDocumentChangedThisFrame,
                "injected tool row document unchanged") &&
         expect(window.creativeDocumentRevisionBeforeFrame == 0U,
                "injected tool row revision before") &&
         expect(window.creativeDocumentRevisionAfterFrame == 0U,
                "injected tool row revision after") &&
         expect(!window.creativeBakedRoomAutoRefreshRequested,
                "injected tool row no auto refresh") &&
         expect(!window.creativeBakedRoomStale,
                "injected tool row not stale");
}

bool inputFrameInjectedClickOnToolMeasureRowDoesNotStaleBakedRoom() {
  iggy3d::FrontendState frontend;
  iggy3d::enterFrontendGameplay(frontend, iggy3d::FrontendAction::NewWorld);
  iggy3d::ProductSaveBridgeResult saves;
  iggy3d::ProductAppOptions options;
  iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::Input;
  std::optional<iggy3d::Session> activeSession;
  activeSession.emplace();
  iggy3d::WorldSetupDraft worldSetupDraft;
  iggy3d::ProductAppWindowState window;
  window.gameplayActive = true;
  markCreativeDocumentWindow(window);
  iggy3d::FrontendSettings settings;
  iggy3d::ProductWindowInputFrameState inputFrame;
  iggy3d::creative::CreativeAppState app;
  iggy3d::creative::Facade& facade = app.facade;
  facade.reset();
  assignValidDocumentId(facade);
  bool closeRequested = false;

  const iggy3d::ProductUiDrawList drawList = defaultCreativeDrawList();
  const iggy3d::UiHitRegion* measureHit =
      findHitRegion(drawList, "creative.row.tools.tool_measure");
  iggy3d::ProductWindowInputClickOverride clickOverride;
  clickOverride.enabled = true;
  if (measureHit != nullptr) {
    clickOverride.click = clickAt(measureHit->rect.x, measureHit->rect.y);
  }

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
      &app,
      &drawList,
      {},
      {},
      0,
      iggy3d::creative::CreativeViewportPickDepthMode::FixedZ,
      clickOverride,
  });

  return expect(measureHit != nullptr, "measure row hit exists") &&
         expect(window.creativeUiCommandAccepted,
                "measure row command accepted") &&
         expect(window.creativeUiCommandChanged,
                "measure row tool changed") &&
         expect(window.creativeUiCommandKind == "set_active_tool",
                "measure row command kind") &&
         expect(window.creativeUiCommandToolAfter == "Measure",
                "measure row tool after") &&
         expect(facade.toolState().activeTool ==
                    iggy3d::creative::Tool::Measure,
                "measure row facade tool") &&
         expect(window.creativeDocumentRevisionObserved,
                "measure row revision observed") &&
         expect(!window.creativeDocumentChangedThisFrame,
                "measure row document unchanged") &&
         expect(window.creativeDocumentRevisionBeforeFrame == 0U,
                "measure row revision before") &&
         expect(window.creativeDocumentRevisionAfterFrame == 0U,
                "measure row revision after") &&
         expect(!window.creativeBakedRoomAutoRefreshRequested,
                "measure row no auto refresh") &&
         expect(!window.creativeBakedRoomStale,
                "measure row not stale");
}

bool inputFrameInjectedClickOnCreateRoomRowCreatesRoomAndSuppressesClick() {
  iggy3d::FrontendState frontend;
  iggy3d::enterFrontendGameplay(frontend, iggy3d::FrontendAction::NewWorld);
  iggy3d::ProductSaveBridgeResult saves;
  iggy3d::ProductAppOptions options;
  iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::Input;
  std::optional<iggy3d::Session> activeSession;
  activeSession.emplace();
  iggy3d::WorldSetupDraft worldSetupDraft;
  iggy3d::ProductAppWindowState window;
  window.gameplayActive = true;
  markCreativeDocumentWindow(window);
  iggy3d::FrontendSettings settings;
  iggy3d::ProductWindowInputFrameState inputFrame;
  iggy3d::creative::CreativeAppState app;
  [[maybe_unused]] iggy3d::creative::Facade& facade = app.facade;
  facade.reset();
  assignValidDocumentId(facade);
  bool closeRequested = false;

  const iggy3d::creative::CreativeUiBuildReceipt ui = facade.buildUiModel();
  iggy3d::ProductCreativeUiDrawListRequest drawRequest;
  drawRequest.model = &ui.model;
  const iggy3d::ProductUiDrawList drawList =
      iggy3d::buildProductCreativeUiDrawList(drawRequest);
  const iggy3d::UiHitRegion* createHit =
      findHitRegion(drawList, "creative.row.create.create_room");

  iggy3d::ProductWindowInputClickOverride clickOverride;
  clickOverride.enabled = true;
  if (createHit != nullptr) {
    clickOverride.click = clickAt(createHit->rect.x, createHit->rect.y);
  }

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
      &app,
      &drawList,
      {},
      {},
      0,
      iggy3d::creative::CreativeViewportPickDepthMode::FixedZ,
      clickOverride,
  });

  const iggy3d::creative::CreativeObject* created =
      facade.findObject(window.creativeUiCommandCreateObjectId);
  const iggy3d::creative::CreativeUiBuildReceipt rebuiltUi =
      facade.buildUiModel();
  iggy3d::ProductCreativeUiDrawListRequest rebuiltDrawRequest;
  rebuiltDrawRequest.model = &rebuiltUi.model;
  const iggy3d::ProductUiDrawList rebuiltDrawList =
      iggy3d::buildProductCreativeUiDrawList(rebuiltDrawRequest);
  const iggy3d::ProductUiPrimitive* rebuiltCreatePrimitive =
      findPrimitive(rebuiltDrawList, "creative.row.create.create_room");
  const iggy3d::RenderReceipt receipt = receiptFor(window);
  const std::string createdObjectId =
      std::to_string(window.creativeUiCommandCreateObjectId);

  return expect(createHit != nullptr, "create injected hit exists") &&
         expect(window.creativeUiInputConsumed,
                "create injected row click consumed") &&
         expect(window.creativeUiInputSemanticId ==
                    "creative.row.create.create_room",
                "create injected row semantic") &&
         expect(window.creativeUiCommandRequested,
                "create injected command requested") &&
         expect(window.creativeUiCommandAccepted,
                "create injected command accepted") &&
         expect(window.creativeUiCommandChanged,
                "create injected command changed") &&
         expect(window.creativeUiCommandKind == "create_object",
                "create injected command kind") &&
         expect(window.creativeUiCommandCreateRequested,
                "create injected create requested") &&
         expect(window.creativeUiCommandCreateAccepted,
                "create injected create accepted") &&
         expect(window.creativeUiCommandCreateChanged,
                "create injected create changed") &&
         expect(window.creativeUiCommandCreateStatus == "Created",
                "create injected create status") &&
         expect(window.creativeUiCommandCreateObjectId != 0U,
                "create injected object id") &&
         expect(window.creativeUiCommandCreateObjectKind == "Room",
                "create injected object kind") &&
         expect(window.creativeUiCommandCreateObjectName == "Room",
                "create injected object name") &&
         expect(window.creativeUiCommandCreateRevisionBefore == 0U,
                "create injected revision before") &&
         expect(window.creativeUiCommandCreateRevisionAfter == 1U,
                "create injected revision after") &&
         expect(window.creativeUiCommandCreateDirtyFlags != 0U,
                "create injected dirty flags") &&
         expect(window.creativeUiCommandCreateMessage == "object_created",
                "create injected create message") &&
         expect(window.creativeUiCommandCreateReasonCode == "object_created",
                "create injected create reason") &&
         expect(window.creativeUiLastClickSeen,
                "create injected sticky click seen") &&
         expect(window.creativeUiLastInputHit,
                "create injected sticky input hit") &&
         expect(window.creativeUiLastInputConsumed,
                "create injected sticky input consumed") &&
         expect(window.creativeUiLastInputSemanticId ==
                    "creative.row.create.create_room",
                "create injected sticky input semantic") &&
         expect(window.creativeUiLastCommandKind == "create_object",
                "create injected sticky command kind") &&
         expect(window.creativeUiLastCommandStatus ==
                    "product_creative_ui_command_applied",
                "create injected sticky command status") &&
         expect(window.creativeUiLastCommandCreateRequested,
                "create injected sticky create requested") &&
         expect(window.creativeUiLastCommandCreateAccepted,
                "create injected sticky create accepted") &&
         expect(window.creativeUiLastCommandCreateChanged,
                "create injected sticky create changed") &&
         expect(window.creativeUiLastCommandCreateObjectId ==
                    window.creativeUiCommandCreateObjectId,
                "create injected sticky object id") &&
         expectReceiptField(receipt,
                            "creative_ui_last_click_seen",
                            "true",
                            "create injected receipt sticky click") &&
         expectReceiptField(receipt,
                            "creative_ui_last_input_semantic_id",
                            "creative.row.create.create_room",
                            "create injected receipt sticky semantic") &&
         expectReceiptField(receipt,
                            "creative_ui_last_command_kind",
                            "create_object",
                            "create injected receipt sticky command") &&
         expectReceiptField(receipt,
                            "creative_ui_last_command_create_object_id",
                            createdObjectId,
                            "create injected receipt sticky object id") &&
         expectReceiptField(receipt,
                            "creative_document_revision_observed",
                            "true",
                            "create injected revision observed receipt") &&
         expectReceiptField(receipt,
                            "creative_document_changed_this_frame",
                            "true",
                            "create injected changed receipt") &&
         expectReceiptField(receipt,
                            "creative_baked_room_stale",
                            "false",
                            "create injected fresh receipt") &&
         expectReceiptField(receipt,
                            "creative_baked_room_stale_status",
                            "creative_baked_room_fresh",
                            "create injected fresh status receipt") &&
         expectReceiptField(receipt,
                            "creative_baked_room_auto_refresh_requested",
                            "true",
                            "create injected auto refresh requested") &&
         expectReceiptField(receipt,
                            "creative_baked_room_auto_refresh_accepted",
                            "true",
                            "create injected auto refresh accepted") &&
         expectReceiptField(
             receipt,
             "creative_baked_room_auto_refresh_cleared_active_room",
             "true",
             "create injected auto refresh cleared") &&
         expectReceiptField(
             receipt,
             "creative_baked_room_auto_refresh_status",
             "product_creative_baked_room_cleared_no_renderable_objects",
             "create injected auto refresh clear status") &&
         expect(facade.document().objectCount() == 1U,
                "create injected object count") &&
         expect(facade.document().revision() == 1U,
                "create injected document revision") &&
         expect(window.creativeDocumentRevisionObserved,
                "create injected revision observed") &&
         expect(window.creativeDocumentChangedThisFrame,
                "create injected document changed") &&
         expect(window.creativeDocumentRevisionBeforeFrame == 0U,
                "create injected frame revision before") &&
         expect(window.creativeDocumentRevisionAfterFrame == 1U,
                "create injected frame revision after") &&
         expect(window.creativeBakedRoomAutoRefreshRequested,
                "create injected auto refresh requested state") &&
         expect(window.creativeBakedRoomAutoRefreshAccepted,
                "create injected auto refresh accepted state") &&
         expect(window.creativeBakedRoomAutoRefreshClearedActiveRoom,
                "create injected auto refresh cleared state") &&
         expect(window.creativeBakedRoomAutoRefreshStatus ==
                    "product_creative_baked_room_cleared_no_renderable_objects",
                "create injected auto refresh status state") &&
         expect(window.creativeBakedRoomAutoRefreshStaticMeshCount == 0U,
                "create injected auto refresh mesh count") &&
         expect(!window.creativeBakedRoomStale,
                "create injected baked room fresh") &&
         expect(window.creativeBakedRoomStaleRevision == 1U,
                "create injected fresh revision") &&
         expect(window.creativeBakedRoomStaleStatus ==
                    "creative_baked_room_fresh",
                "create injected fresh status") &&
         expect(!window.activeRoom.loaded,
                "create injected metadata clears active room") &&
         expect(window.activeRoom.status ==
                    "product_creative_baked_room_cleared_no_renderable_objects",
                "create injected active room clear status") &&
         expect(!window.activeRoomCollision.ready,
                "create injected collision unavailable") &&
         expect(created != nullptr, "create injected object exists") &&
         expect(created != nullptr &&
                    created->kind ==
                        iggy3d::creative::CreativeObjectKind::Room,
                "create injected object room") &&
         expect(created != nullptr && created->bounds.min.x == 0.0 &&
                    created->bounds.min.y == 0.0 &&
                    created->bounds.min.z == 0.0,
                "create injected bounds min") &&
         expect(created != nullptr && created->bounds.max.x == 10.0 &&
                    created->bounds.max.y == 4.0 &&
                    created->bounds.max.z == 10.0,
                "create injected bounds max") &&
         expect(created != nullptr && created->visible,
                "create injected visible") &&
         expect(created != nullptr && !created->locked,
                "create injected unlocked") &&
         expect(facade.toolState().activeTool ==
                    iggy3d::creative::Tool::Select,
                "create injected active tool unchanged") &&
         expect(facade.state().tool == iggy3d::creative::Tool::Select,
                "create injected old tool unchanged") &&
         expect(facade.selectionState().selectedTarget.value ==
                    iggy3d::creative::kInvalidId,
                "create injected selection invalid") &&
         expect(!facade.measurementState().active,
                "create injected no measurement") &&
         expect(window.creativeUiInputDownstreamClickSuppressed,
                "create injected downstream suppressed") &&
         expect(window.creativeViewportPickClickSuppressed,
                "create injected viewport suppressed") &&
         expect(!window.creativeViewportPickPicked,
                "create injected viewport did not pick") &&
         expect(window.creativeViewportPickStatus ==
                    "product_creative_viewport_pick_no_click",
                "create injected viewport no-click") &&
         expect(rebuiltCreatePrimitive != nullptr,
                "create injected rebuilt create row") &&
         expect(rebuiltCreatePrimitive != nullptr &&
                    rebuiltCreatePrimitive->text == "Create Room",
                "create injected rebuilt text");
}

bool inputFrameInjectedClickOnCreateCrateRowAutoRefreshesBakedRoom() {
  iggy3d::FrontendState frontend;
  iggy3d::enterFrontendGameplay(frontend, iggy3d::FrontendAction::NewWorld);
  iggy3d::ProductSaveBridgeResult saves;
  iggy3d::ProductAppOptions options;
  iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::Input;
  std::optional<iggy3d::Session> activeSession;
  activeSession.emplace();
  iggy3d::WorldSetupDraft worldSetupDraft;
  iggy3d::ProductAppWindowState window;
  window.gameplayActive = true;
  markCreativeDocumentWindow(window);
  iggy3d::FrontendSettings settings;
  iggy3d::ProductWindowInputFrameState inputFrame;
  iggy3d::creative::CreativeAppState app;
  iggy3d::creative::Facade& facade = app.facade;
  facade.reset();
  assignValidDocumentId(facade);
  bool closeRequested = false;

  const iggy3d::creative::CreativeUiBuildReceipt ui = facade.buildUiModel();
  iggy3d::ProductCreativeUiDrawListRequest drawRequest;
  drawRequest.model = &ui.model;
  const iggy3d::ProductUiDrawList drawList =
      iggy3d::buildProductCreativeUiDrawList(drawRequest);
  const iggy3d::UiHitRegion* createHit =
      findHitRegion(drawList, "creative.row.create.create_crate");

  iggy3d::ProductWindowInputClickOverride clickOverride;
  clickOverride.enabled = true;
  if (createHit != nullptr) {
    clickOverride.click = clickAt(createHit->rect.x, createHit->rect.y);
  }

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
      &app,
      &drawList,
      {},
      {},
      0,
      iggy3d::creative::CreativeViewportPickDepthMode::FixedZ,
      clickOverride,
  });

  const iggy3d::creative::CreativeObject* created =
      facade.findObject(window.creativeUiCommandCreateObjectId);
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expect(createHit != nullptr, "crate injected hit exists") &&
         expect(window.creativeUiInputConsumed,
                "crate injected row click consumed") &&
         expect(window.creativeUiInputSemanticId ==
                    "creative.row.create.create_crate",
                "crate injected row semantic") &&
         expect(window.creativeUiCommandAccepted,
                "crate injected command accepted") &&
         expect(window.creativeUiCommandChanged,
                "crate injected command changed") &&
         expect(window.creativeUiCommandKind == "create_object",
                "crate injected command kind") &&
         expect(window.creativeUiCommandCreateAccepted,
                "crate injected create accepted") &&
         expect(window.creativeUiCommandCreateChanged,
                "crate injected create changed") &&
         expect(window.creativeUiCommandCreateObjectKind == "Crate",
                "crate injected object kind") &&
         expect(facade.document().objectCount() == 1U,
                "crate injected object count") &&
         expect(facade.document().revision() == 1U,
                "crate injected document revision") &&
         expect(created != nullptr, "crate injected object exists") &&
         expect(created != nullptr &&
                    created->kind ==
                        iggy3d::creative::CreativeObjectKind::Crate,
                "crate injected object crate") &&
         expect(window.creativeDocumentRevisionObserved,
                "crate injected revision observed") &&
         expect(window.creativeDocumentChangedThisFrame,
                "crate injected document changed") &&
         expect(window.creativeDocumentRevisionBeforeFrame == 0U,
                "crate injected revision before") &&
         expect(window.creativeDocumentRevisionAfterFrame == 1U,
                "crate injected revision after") &&
         expect(window.creativeBakedRoomAutoRefreshRequested,
                "crate injected auto refresh requested") &&
         expect(window.creativeBakedRoomAutoRefreshAccepted,
                "crate injected auto refresh accepted") &&
         expect(!window.creativeBakedRoomAutoRefreshClearedActiveRoom,
                "crate injected auto refresh did not clear") &&
         expect(window.creativeBakedRoomAutoRefreshStatus ==
                    "product_creative_baked_room_refreshed",
                "crate injected auto refresh status") &&
         expect(window.creativeBakedRoomAutoRefreshStaticMeshCount == 1U,
                "crate injected auto refresh mesh count") &&
         expect(window.creativeBakedRoomAutoRefreshAnchorCount == 0U,
                "crate injected auto refresh anchor count") &&
         expect(window.creativeBakedRoomAutoRefreshSpatialSurfaceCount == 2U,
                "crate injected auto refresh surface count") &&
         expect(window.creativeBakedRoomAutoRefreshCollisionReady,
                "crate injected auto refresh collision ready") &&
         expect(window.creativeBakedRoomAutoRefreshCollisionQuerySurfaceCount == 2U,
                "crate injected auto refresh collision query count") &&
         expect(!window.creativeBakedRoomStale,
                "crate injected baked room fresh") &&
         expect(window.creativeBakedRoomStaleStatus ==
                    "creative_baked_room_fresh",
                "crate injected stale status fresh") &&
         expect(window.activeRoom.loaded,
                "crate injected active room loaded") &&
         expect(window.activeRoom.staticMeshCount == 1U,
                "crate injected active room mesh count") &&
         expect(window.activeRoom.spatialSurfaceCount == 2U,
                "crate injected active room surface count") &&
         expect(window.activeRoomCollision.ready,
                "crate injected collision ready") &&
         expect(window.activeRoomCollision.querySurfaceCount == 2U,
                "crate injected collision query count") &&
         expectReceiptField(receipt,
                            "creative_baked_room_auto_refresh_requested",
                            "true",
                            "crate injected receipt auto requested") &&
         expectReceiptField(receipt,
                            "creative_baked_room_auto_refresh_accepted",
                            "true",
                            "crate injected receipt auto accepted") &&
         expectReceiptField(receipt,
                            "creative_baked_room_auto_refresh_status",
                            "product_creative_baked_room_refreshed",
                            "crate injected receipt auto status") &&
         expectReceiptField(receipt,
                            "creative_baked_room_stale",
                            "false",
                            "crate injected receipt fresh");
}

bool inputFrameInjectedClickWithoutCreativeUiDrawListReachesCreativeTool() {
  iggy3d::FrontendState frontend;
  iggy3d::enterFrontendGameplay(frontend, iggy3d::FrontendAction::NewWorld);
  iggy3d::ProductSaveBridgeResult saves;
  iggy3d::ProductAppOptions options;
  iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::Input;
  std::optional<iggy3d::Session> activeSession;
  activeSession.emplace();
  iggy3d::WorldSetupDraft worldSetupDraft;
  iggy3d::ProductAppWindowState window;
  window.gameplayActive = true;
  markCreativeDocumentWindow(window);
  iggy3d::FrontendSettings settings;
  iggy3d::ProductWindowInputFrameState inputFrame;
  iggy3d::creative::CreativeAppState app;
  [[maybe_unused]] iggy3d::creative::Facade& facade = app.facade;
  facade.reset();
  static_cast<void>(facade.setActiveTool(iggy3d::creative::Tool::Measure));
  bool closeRequested = false;

  iggy3d::ProductWindowInputClickOverride clickOverride;
  clickOverride.enabled = true;
  clickOverride.click = clickAt(80.0F, 96.0F);

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
      &app,
      nullptr,
      {},
      {},
      0,
      iggy3d::creative::CreativeViewportPickDepthMode::FixedZ,
      clickOverride,
  });

  return expect(window.creativeUiInputRequested,
                "injected missing draw requested") &&
         expect(window.creativeUiInputClickPresent,
                "injected missing draw click") &&
         expect(!window.creativeUiInputDrawListAvailable,
                "injected missing draw unavailable") &&
         expect(window.creativeUiInputStatus ==
                    "product_creative_ui_input_draw_list_missing",
                "injected missing draw status") &&
         expect(!window.creativeUiInputConsumed,
                "injected missing draw not consumed") &&
         expect(!window.creativeUiInputDownstreamClickSuppressed,
                "injected missing draw not suppressed") &&
         expect(window.creativeViewportPickStatus ==
                    "product_creative_viewport_pick_source_empty",
                "injected missing draw viewport source empty") &&
         expect(facade.measurementState().active,
                "injected missing draw measurement active") &&
         expect(facade.measurementState().hasMeasurement,
                "injected missing draw measurement exists") &&
         expect(facade.measurementState().startPoint.x == 80.0,
                "injected missing draw measurement x") &&
         expect(facade.measurementState().startPoint.y == 96.0,
                "injected missing draw measurement y");
}

}  // namespace

int main() {
  const bool ok = noClickDoesNotRouteAndPreservesDrawListAvailability() &&
                  clickedNullDrawListFailsClosed() &&
                  clickedNotReadyDrawListRoutesAndReportsNotReady() &&
                  clickedReadyCreativeRowConsumes() &&
                  renderableOverlayInputConsumesReadyCreativeRow() &&
                  unrenderableOverlayInputDoesNotSuppressToolClick() &&
                  clickedOutsideReadyDrawListMisses() &&
                  disabledCreativeRowReportsHitDisabled() &&
                  receiptCopiesKnownRegionIndex() &&
                  downstreamNoClickDoesNotSuppress() &&
                  downstreamSuppressesCreativeConsumedClick() &&
                  downstreamHigherPrioritySuppressesClick() &&
                  downstreamPassthroughWhenCreativeDoesNotConsume() &&
                  defaultWindowReceiptFieldsAreNotRequested() &&
                  recorderCopiesNoClickReceipt() &&
                  recorderCopiesConsumedCreativeRowReceipt() &&
                  recorderPreservesStickyClickAndCommandAcrossNoClickFrame() &&
                  recorderCopiesSuppressedDownstreamClickReceipt() &&
                  recorderLeavesOtherReceiptFieldsUntouched() &&
                  inputFrameNoClickNullDrawListRecordsNoClick() &&
                  inputFrameInjectedClickOnToolSelectRowSetsToolAndSuppressesClick() &&
                  inputFrameInjectedClickOnToolMeasureRowDoesNotStaleBakedRoom() &&
                  inputFrameInjectedClickOnCreateRoomRowCreatesRoomAndSuppressesClick() &&
                  inputFrameInjectedClickOnCreateCrateRowAutoRefreshesBakedRoom() &&
                  inputFrameInjectedClickWithoutCreativeUiDrawListReachesCreativeTool();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
