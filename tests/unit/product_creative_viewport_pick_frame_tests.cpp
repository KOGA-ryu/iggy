#include "app/iggy3d/window/CreativeViewportPickFrame.hpp"

#include "app/frontend/WorldSetupModel.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/window/InputFrame.hpp"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

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

cr::CreativeViewportPickViewport viewport() {
  return {0.0F, 0.0F, 400.0F, 400.0F};
}

cr::CreativeSpatialProjectionRequest projectionRequest() {
  cr::CreativeSpatialProjectionRequest request;
  request.gridSize = {4, 4, 2};
  request.cellSize = 1.0;
  request.clampToGrid = true;
  request.includeAuthoringOnly = false;
  return request;
}

iggy3d::ProductAppWindowState creativeWindow() {
  iggy3d::ProductAppWindowState window;
  window.interactionMode = iggy3d::ProductInteractionMode::Creative;
  return window;
}

cr::Facade facadeWithRoom() {
  cr::Facade facade;
  cr::CreateRoomCommand command;
  command.name = "Room";
  command.bounds.min = {1.0, 2.0, 1.0};
  command.bounds.max = {2.0, 3.0, 2.0};
  (void)facade.createRoom(command);
  return facade;
}

cr::CreativeToolInputPacket pointerPress(cr::Id targetId) {
  cr::CreativeToolInputPacket input;
  input.kind = cr::CreativeToolInputKind::PointerPress;
  input.pointer.button = cr::CreativeToolPointerButton::Primary;
  input.pointer.target.value = targetId;
  return input;
}

void selectTarget(cr::Facade& facade, cr::CreativeObjectId objectId) {
  static_cast<void>(facade.setActiveTool(cr::Tool::Select));
  static_cast<void>(facade.dispatchToolInput(
      pointerPress(static_cast<cr::Id>(objectId))));
}

iggy3d::ProductCreativeViewportPickFrameRequest baseRequest(
    iggy3d::ProductAppWindowState& window,
    cr::Facade& facade) {
  iggy3d::ProductCreativeViewportPickFrameRequest request;
  request.window = &window;
  request.facade = &facade;
  request.click = clickAt(150.0F, 250.0F);
  request.viewport = viewport();
  request.projectionRequest = projectionRequest();
  request.z = 1;
  return request;
}

bool activeRuleUsesOnlyInteractionMode() {
  iggy3d::ProductAppWindowState player;
  player.interactionMode = iggy3d::ProductInteractionMode::Player;
  iggy3d::ProductAppWindowState creative = creativeWindow();

  return expect(!iggy3d::productCreativeViewportPickActiveForWindow(player),
                "player inactive") &&
         expect(iggy3d::productCreativeViewportPickActiveForWindow(creative),
                "creative active");
}

bool nullWindowFailsClosed() {
  cr::Facade facade = facadeWithRoom();
  iggy3d::ProductCreativeViewportPickFrameRequest request;
  request.facade = &facade;
  request.click = clickAt(150.0F, 250.0F);
  request.viewport = viewport();
  request.projectionRequest = projectionRequest();
  request.z = 1;

  const iggy3d::ProductCreativeViewportPickFrameReceipt receipt =
      iggy3d::routeProductCreativeViewportPickFrame(request);

  return expect(receipt.requested, "null window requested") &&
         expect(!receipt.active, "null window inactive") &&
         expect(!receipt.picked, "null window no pick") &&
         expect(receipt.status ==
                    "product_creative_viewport_pick_window_missing",
                "null window status");
}

bool inactiveWindowDoesNotPick() {
  iggy3d::ProductAppWindowState window;
  cr::Facade facade = facadeWithRoom();
  iggy3d::ProductCreativeViewportPickFrameRequest request =
      baseRequest(window, facade);

  const iggy3d::ProductCreativeViewportPickFrameReceipt receipt =
      iggy3d::routeProductCreativeViewportPickFrame(request);

  return expect(receipt.requested, "inactive requested") &&
         expect(!receipt.active, "inactive active false") &&
         expect(receipt.clickPresent, "inactive click copied") &&
         expect(!receipt.facadeAvailable,
                "inactive facade not consulted") &&
         expect(receipt.status == "product_creative_viewport_pick_inactive",
                "inactive status");
}

bool noClickDoesNotPick() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade = facadeWithRoom();
  iggy3d::ProductCreativeViewportPickFrameRequest request =
      baseRequest(window, facade);
  request.click = {};

  const iggy3d::ProductCreativeViewportPickFrameReceipt receipt =
      iggy3d::routeProductCreativeViewportPickFrame(request);

  return expect(receipt.active, "no click active") &&
         expect(!receipt.clickPresent, "no click absent") &&
         expect(!receipt.facadeAvailable, "no click facade not consulted") &&
         expect(receipt.status == "product_creative_viewport_pick_no_click",
                "no click status");
}

bool suppressedClickDoesNotPick() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade = facadeWithRoom();
  iggy3d::ProductCreativeViewportPickFrameRequest request =
      baseRequest(window, facade);
  request.downstreamClickSuppressed = true;

  const iggy3d::ProductCreativeViewportPickFrameReceipt receipt =
      iggy3d::routeProductCreativeViewportPickFrame(request);

  return expect(receipt.downstreamClickSuppressed, "suppressed copied") &&
         expect(!receipt.facadeAvailable,
                "suppressed facade not consulted") &&
         expect(receipt.status ==
                    "product_creative_viewport_pick_click_suppressed",
                "suppressed status");
}

bool missingFacadeDoesNotPick() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  iggy3d::ProductCreativeViewportPickFrameRequest request;
  request.window = &window;
  request.click = clickAt(150.0F, 250.0F);
  request.viewport = viewport();
  request.projectionRequest = projectionRequest();
  request.z = 1;

  const iggy3d::ProductCreativeViewportPickFrameReceipt receipt =
      iggy3d::routeProductCreativeViewportPickFrame(request);

  return expect(receipt.active, "missing facade active") &&
         expect(!receipt.facadeAvailable, "missing facade available false") &&
         expect(receipt.status ==
                    "product_creative_viewport_pick_facade_missing",
                "missing facade status");
}

bool emptyFacadeDocumentReportsSourceEmpty() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade;
  iggy3d::ProductCreativeViewportPickFrameRequest request =
      baseRequest(window, facade);

  const iggy3d::ProductCreativeViewportPickFrameReceipt receipt =
      iggy3d::routeProductCreativeViewportPickFrame(request);

  return expect(receipt.facadeAvailable, "empty facade available") &&
         expect(!receipt.sourceAvailable, "empty source unavailable") &&
         expect(receipt.objectCount == 0U, "empty object count") &&
         expect(receipt.status ==
                    "product_creative_viewport_pick_source_empty",
                "empty source status");
}

bool facadeRoomProjectsAndHits() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade = facadeWithRoom();
  const cr::CreativeObjectId roomId = facade.document().objects()[0].id;
  iggy3d::ProductCreativeViewportPickFrameRequest request =
      baseRequest(window, facade);

  const iggy3d::ProductCreativeViewportPickFrameReceipt receipt =
      iggy3d::routeProductCreativeViewportPickFrame(request);

  return expect(receipt.facadeAvailable, "hit facade available") &&
         expect(receipt.sourceAvailable, "hit source available") &&
         expect(receipt.projected, "hit projected") &&
         expect(receipt.objectCount == 1U, "hit object count") &&
         expect(receipt.projectionCellCount == 1U, "hit cell count") &&
         expect(receipt.picked, "hit picked") &&
         expect(receipt.pickStatus == cr::CreativeViewportPickStatus::Hit,
                "hit pick status") &&
         expect(receipt.objectId == roomId, "hit object id") &&
         expect(receipt.objectKind == cr::CreativeObjectKind::Room,
                "hit kind") &&
         expect(receipt.occupancyKind ==
                    cr::CreativeSpatialOccupancyKind::Structural,
                "hit occupancy") &&
         expect(receipt.coord.x == 1 && receipt.coord.y == 2 &&
                    receipt.coord.z == 1,
                "hit coord") &&
         expect(receipt.gridIndex == 25U, "hit grid index") &&
         expect(receipt.target.value == roomId, "hit target") &&
         expect(receipt.cellIndex == 0U, "hit cell index") &&
         expect(receipt.pickMessage == "hit", "hit message") &&
         expect(receipt.status == "product_creative_viewport_pick_hit",
                "hit status");
}

bool defaultDepthModeStaysFixedZ() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade = facadeWithRoom();
  iggy3d::ProductCreativeViewportPickFrameRequest request =
      baseRequest(window, facade);
  request.z = 0;

  const iggy3d::ProductCreativeViewportPickFrameReceipt receipt =
      iggy3d::routeProductCreativeViewportPickFrame(request);

  return expect(receipt.projected, "fixed z projected") &&
         expect(receipt.projectionCellCount == 1U, "fixed z cell count") &&
         expect(!receipt.picked, "fixed z not picked") &&
         expect(receipt.pickStatus == cr::CreativeViewportPickStatus::Miss,
                "fixed z pick status") &&
         expect(receipt.coord.x == 1 && receipt.coord.y == 2 &&
                    receipt.coord.z == 0,
                "fixed z miss coord") &&
         expect(receipt.gridIndex == 9U, "fixed z miss index") &&
         expect(receipt.status == "product_creative_viewport_pick_miss",
                "fixed z miss status");
}

bool highestZDepthModePicksRoomAtZOneWithRequestZZero() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade = facadeWithRoom();
  const cr::CreativeObjectId roomId = facade.document().objects()[0].id;
  iggy3d::ProductCreativeViewportPickFrameRequest request =
      baseRequest(window, facade);
  request.z = 0;
  request.depthMode = cr::CreativeViewportPickDepthMode::HighestZFirst;

  const iggy3d::ProductCreativeViewportPickFrameReceipt receipt =
      iggy3d::routeProductCreativeViewportPickFrame(request);

  return expect(receipt.projected, "highest frame projected") &&
         expect(receipt.projectionCellCount == 1U,
                "highest frame cell count") &&
         expect(receipt.picked, "highest frame picked") &&
         expect(receipt.pickStatus == cr::CreativeViewportPickStatus::Hit,
                "highest frame pick hit") &&
         expect(receipt.objectId == roomId, "highest frame object id") &&
         expect(receipt.coord.x == 1 && receipt.coord.y == 2 &&
                    receipt.coord.z == 1,
                "highest frame coord") &&
         expect(receipt.gridIndex == 25U, "highest frame grid index") &&
         expect(receipt.target.value == roomId, "highest frame target") &&
         expect(receipt.status == "product_creative_viewport_pick_hit",
                "highest frame status");
}

bool hiddenRoomDoesNotProduceViewportPickHit() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade = facadeWithRoom();
  const cr::CreativeObjectId roomId = facade.document().objects()[0].id;
  selectTarget(facade, roomId);
  const cr::CreativeFacadeMutationReceipt mutation =
      facade.toggleSelectedObjectVisibility();
  const cr::CreativeObject* room = facade.findObject(roomId);
  iggy3d::ProductCreativeViewportPickFrameRequest request =
      baseRequest(window, facade);

  const iggy3d::ProductCreativeViewportPickFrameReceipt receipt =
      iggy3d::routeProductCreativeViewportPickFrame(request);

  return expect(mutation.accepted && mutation.changed,
                "hidden pick setup mutation changed") &&
         expect(room != nullptr && !room->visible,
                "hidden pick setup room hidden") &&
         expect(facade.selectionState().selectedTarget.value == roomId,
                "hidden pick selection preserved") &&
         expect(receipt.facadeAvailable, "hidden pick facade available") &&
         expect(receipt.sourceAvailable, "hidden pick source available") &&
         expect(receipt.objectCount == 1U, "hidden pick object count") &&
         expect(!receipt.projected, "hidden pick not projected") &&
         expect(receipt.projectionCellCount == 0U,
                "hidden pick zero cells") &&
         expect(!receipt.picked, "hidden pick not picked") &&
         expect(receipt.target.value == cr::kInvalidId,
                "hidden pick invalid target") &&
         expect(receipt.status ==
                    "product_creative_viewport_pick_projection_empty",
                "hidden pick status");
}

bool missWithProjectedCellsReportsMiss() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade = facadeWithRoom();
  iggy3d::ProductCreativeViewportPickFrameRequest request =
      baseRequest(window, facade);
  request.click = clickAt(50.0F, 50.0F);
  request.z = 0;

  const iggy3d::ProductCreativeViewportPickFrameReceipt receipt =
      iggy3d::routeProductCreativeViewportPickFrame(request);

  return expect(receipt.projected, "miss projected") &&
         expect(receipt.projectionCellCount == 1U, "miss projected cells") &&
         expect(!receipt.picked, "miss not picked") &&
         expect(receipt.pickStatus == cr::CreativeViewportPickStatus::Miss,
                "miss pick status") &&
         expect(receipt.pickMessage == "miss", "miss message") &&
         expect(receipt.status == "product_creative_viewport_pick_miss",
                "miss status");
}

bool invalidViewportPropagatesThroughPick() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade = facadeWithRoom();
  iggy3d::ProductCreativeViewportPickFrameRequest request =
      baseRequest(window, facade);
  request.viewport.width = 0.0F;

  const iggy3d::ProductCreativeViewportPickFrameReceipt receipt =
      iggy3d::routeProductCreativeViewportPickFrame(request);

  return expect(receipt.projected, "invalid viewport still projected") &&
         expect(!receipt.picked, "invalid viewport not picked") &&
         expect(receipt.pickStatus ==
                    cr::CreativeViewportPickStatus::InvalidViewport,
                "invalid viewport pick status") &&
         expect(receipt.status ==
                    "product_creative_viewport_pick_invalid_viewport",
                "invalid viewport status");
}

bool invalidGridPropagatesThroughPick() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade = facadeWithRoom();
  iggy3d::ProductCreativeViewportPickFrameRequest request =
      baseRequest(window, facade);
  request.projectionRequest.gridSize = {0, 4, 2};

  const iggy3d::ProductCreativeViewportPickFrameReceipt receipt =
      iggy3d::routeProductCreativeViewportPickFrame(request);

  return expect(!receipt.projected, "invalid grid not projected") &&
         expect(receipt.projectionCellCount == 0U,
                "invalid grid no cells") &&
         expect(!receipt.picked, "invalid grid not picked") &&
         expect(receipt.pickStatus == cr::CreativeViewportPickStatus::InvalidGrid,
                "invalid grid pick status") &&
         expect(receipt.status ==
                    "product_creative_viewport_pick_invalid_grid",
                "invalid grid status");
}

bool defaultWindowReceiptFieldsAreNotRequested() {
  const iggy3d::ProductAppWindowState window;
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expectReceiptField(receipt,
                            "creative_viewport_pick_requested",
                            "false",
                            "default requested") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_active",
                            "false",
                            "default active") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_click_present",
                            "false",
                            "default click") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_click_suppressed",
                            "false",
                            "default suppressed") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_facade_available",
                            "false",
                            "default facade") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_source_available",
                            "false",
                            "default source") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_projected",
                            "false",
                            "default projected") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_picked",
                            "false",
                            "default picked") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_object_count",
                            "0",
                            "default object count") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_projection_cell_count",
                            "0",
                            "default cell count") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_status",
                            "creative_viewport_pick_not_requested",
                            "default status") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_reason_code",
                            "creative_viewport_pick_not_requested",
                            "default reason") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_pick_status",
                            "Unknown",
                            "default pick status") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_message",
                            "none",
                            "default message") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_coord_x",
                            "0",
                            "default coord x") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_grid_index",
                            "0",
                            "default grid index") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_object_id",
                            "0",
                            "default object id") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_object_kind",
                            "Unknown",
                            "default object kind") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_occupancy_kind",
                            "Unknown",
                            "default occupancy") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_target",
                            "0",
                            "default target") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_cell_index",
                            "0",
                            "default cell index");
}

bool recorderCopiesHitReceiptFields() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade = facadeWithRoom();
  const cr::CreativeObjectId roomId = facade.document().objects()[0].id;
  const iggy3d::ProductCreativeViewportPickFrameReceipt pickReceipt =
      iggy3d::routeProductCreativeViewportPickFrame(baseRequest(window,
                                                                facade));

  iggy3d::ProductAppWindowState receiptWindow;
  iggy3d::recordProductCreativeViewportPickFrame(receiptWindow, pickReceipt);
  const iggy3d::RenderReceipt receipt = receiptFor(receiptWindow);

  return expectReceiptField(receipt,
                            "creative_viewport_pick_requested",
                            "true",
                            "hit receipt requested") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_active",
                            "true",
                            "hit receipt active") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_click_present",
                            "true",
                            "hit receipt click") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_facade_available",
                            "true",
                            "hit receipt facade") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_source_available",
                            "true",
                            "hit receipt source") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_projected",
                            "true",
                            "hit receipt projected") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_picked",
                            "true",
                            "hit receipt picked") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_object_count",
                            "1",
                            "hit receipt object count") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_projection_cell_count",
                            "1",
                            "hit receipt cell count") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_status",
                            "product_creative_viewport_pick_hit",
                            "hit receipt status") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_reason_code",
                            "product_creative_viewport_pick_hit",
                            "hit receipt reason") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_pick_status",
                            "Hit",
                            "hit receipt pick status") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_message",
                            "hit",
                            "hit receipt message") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_coord_x",
                            "1",
                            "hit receipt coord x") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_coord_y",
                            "2",
                            "hit receipt coord y") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_coord_z",
                            "1",
                            "hit receipt coord z") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_grid_index",
                            "25",
                            "hit receipt grid index") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_object_id",
                            std::to_string(roomId),
                            "hit receipt object id") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_object_kind",
                            "Room",
                            "hit receipt object kind") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_occupancy_kind",
                            "Structural",
                            "hit receipt occupancy") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_target",
                            std::to_string(roomId),
                            "hit receipt target") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_cell_index",
                            "0",
                            "hit receipt cell index");
}

bool recorderLeavesOtherFieldsUntouched() {
  iggy3d::ProductAppWindowState window;
  window.status = "window_before";
  window.productVulkanMenuUiReady = true;
  window.productVulkanMenuUiStatus = "vulkan_before";
  window.creativeUiProjectionRequested = true;
  window.creativeUiProjectionStatus = "projection_before";
  window.creativeUiInputRequested = true;
  window.creativeUiInputStatus = "input_before";
  window.creativeUiInputDownstreamClickRequested = true;
  window.creativeUiInputDownstreamClickStatus = "downstream_before";

  iggy3d::ProductCreativeViewportPickFrameReceipt receipt;
  receipt.requested = true;
  receipt.status = "product_creative_viewport_pick_hit";
  receipt.reasonCode = receipt.status;
  iggy3d::recordProductCreativeViewportPickFrame(window, receipt);

  return expect(window.status == "window_before", "window status kept") &&
         expect(window.productVulkanMenuUiReady, "vulkan ready kept") &&
         expect(window.productVulkanMenuUiStatus == "vulkan_before",
                "vulkan status kept") &&
         expect(window.creativeUiProjectionRequested,
                "projection requested kept") &&
         expect(window.creativeUiProjectionStatus == "projection_before",
                "projection status kept") &&
         expect(window.creativeUiInputRequested, "input requested kept") &&
         expect(window.creativeUiInputStatus == "input_before",
                "input status kept") &&
         expect(window.creativeUiInputDownstreamClickRequested,
                "downstream requested kept") &&
         expect(window.creativeUiInputDownstreamClickStatus ==
                    "downstream_before",
                "downstream status kept");
}

bool inputFrameNoWindowNoClickRecordsInactiveViewportPick() {
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

  return expect(window.creativeViewportPickRequested,
                "input frame pick requested") &&
         expect(!window.creativeViewportPickActive,
                "input frame pick inactive") &&
         expect(window.creativeViewportPickStatus ==
                    "product_creative_viewport_pick_inactive",
                "input frame pick status");
}

}  // namespace

int main() {
  const bool ok =
      activeRuleUsesOnlyInteractionMode() &&
      nullWindowFailsClosed() &&
      inactiveWindowDoesNotPick() &&
      noClickDoesNotPick() &&
      suppressedClickDoesNotPick() &&
      missingFacadeDoesNotPick() &&
      emptyFacadeDocumentReportsSourceEmpty() &&
      facadeRoomProjectsAndHits() &&
      defaultDepthModeStaysFixedZ() &&
      highestZDepthModePicksRoomAtZOneWithRequestZZero() &&
      hiddenRoomDoesNotProduceViewportPickHit() &&
      missWithProjectedCellsReportsMiss() &&
      invalidViewportPropagatesThroughPick() &&
      invalidGridPropagatesThroughPick() &&
      defaultWindowReceiptFieldsAreNotRequested() &&
      recorderCopiesHitReceiptFields() &&
      recorderLeavesOtherFieldsUntouched() &&
      inputFrameNoWindowNoClickRecordsInactiveViewportPick();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
