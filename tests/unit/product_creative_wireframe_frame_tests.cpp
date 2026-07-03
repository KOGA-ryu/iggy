#include "app/iggy3d/window/CreativeWireframeFrame.hpp"

#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/window/CreativeUiCommandFrame.hpp"

#include <cstdlib>
#include <iostream>
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

iggy3d::ProductAppWindowState creativeWindow() {
  iggy3d::ProductAppWindowState window;
  window.interactionMode = iggy3d::ProductInteractionMode::Creative;
  return window;
}

cr::CreativeSpatialProjectionRequest projectionRequest() {
  cr::CreativeSpatialProjectionRequest request;
  request.gridSize = {64, 64, 16};
  request.cellSize = 1.0;
  request.clampToGrid = true;
  request.includeAuthoringOnly = false;
  return request;
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

iggy3d::ProductCreativeWireframeFrameRequest baseRequest(
    iggy3d::ProductAppWindowState& window,
    cr::Facade& facade) {
  iggy3d::ProductCreativeWireframeFrameRequest request;
  request.window = &window;
  request.facade = &facade;
  request.projectionRequest = projectionRequest();
  return request;
}

cr::CreativeObjectId createRoom(cr::Facade& facade) {
  const cr::CreativeDocumentCreateReceipt receipt =
      facade.createDocumentObject(cr::CreativeObjectKind::Room);
  return receipt.objectId;
}

bool activeRuleUsesOnlyInteractionMode() {
  iggy3d::ProductAppWindowState player;
  player.interactionMode = iggy3d::ProductInteractionMode::Player;
  iggy3d::ProductAppWindowState creative = creativeWindow();

  return expect(!iggy3d::productCreativeWireframeFrameActiveForWindow(player),
                "player wireframe inactive") &&
         expect(iggy3d::productCreativeWireframeFrameActiveForWindow(creative),
                "creative wireframe active");
}

bool inactiveWindowDoesNotBuild() {
  iggy3d::ProductAppWindowState window;
  cr::Facade facade;
  const iggy3d::ProductCreativeWireframeFrameReceipt receipt =
      iggy3d::routeProductCreativeWireframeFrame(baseRequest(window, facade));

  return expect(receipt.requested, "inactive requested") &&
         expect(!receipt.active, "inactive active false") &&
         expect(!receipt.facadeAvailable,
                "inactive facade not consulted") &&
         expect(receipt.segmentCount == 0U, "inactive no segments") &&
         expect(receipt.status ==
                    "product_creative_wireframe_frame_inactive",
                "inactive status");
}

bool nullFacadeFailsClosed() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  iggy3d::ProductCreativeWireframeFrameRequest request;
  request.window = &window;
  request.projectionRequest = projectionRequest();

  const iggy3d::ProductCreativeWireframeFrameReceipt receipt =
      iggy3d::routeProductCreativeWireframeFrame(request);

  return expect(receipt.requested, "null facade requested") &&
         expect(receipt.active, "null facade active") &&
         expect(!receipt.facadeAvailable, "null facade unavailable") &&
         expect(!receipt.documentAvailable,
                "null facade document unavailable") &&
         expect(receipt.status ==
                    "product_creative_wireframe_frame_facade_missing",
                "null facade status");
}

bool emptyCreativeDocumentBuildsZeroSegments() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade;

  const iggy3d::ProductCreativeWireframeFrameReceipt receipt =
      iggy3d::routeProductCreativeWireframeFrame(baseRequest(window, facade));

  return expect(receipt.facadeAvailable, "empty facade available") &&
         expect(receipt.documentAvailable, "empty document available") &&
         expect(!receipt.sourceAvailable, "empty source unavailable") &&
         expect(receipt.objectCount == 0U, "empty object count") &&
         expect(receipt.visibleObjectCount == 0U,
                "empty visible object count") &&
         expect(receipt.itemCount == 0U, "empty item count") &&
         expect(receipt.segmentCount == 0U, "empty segment count") &&
         expect(receipt.wireframeStatus ==
                    cr::CreativeDocumentWireframeStatus::EmptySource,
                "empty wireframe status") &&
         expect(receipt.segmentStatus ==
                    cr::CreativeDocumentWireframeSegmentStatus::EmptySource,
                "empty segment status") &&
         expect(receipt.status ==
                    "product_creative_wireframe_frame_source_empty",
                "empty status");
}

bool oneRoomBuildsTwelveSegments() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade;
  const cr::CreativeObjectId roomId = createRoom(facade);

  const iggy3d::ProductCreativeWireframeFrameReceipt receipt =
      iggy3d::routeProductCreativeWireframeFrame(baseRequest(window, facade));

  return expect(roomId != cr::kInvalidObjectId, "room created") &&
         expect(receipt.sourceAvailable, "room source available") &&
         expect(receipt.objectCount == 1U, "room object count") &&
         expect(receipt.visibleObjectCount == 1U,
                "room visible object count") &&
         expect(receipt.itemCount == 1U, "room item count") &&
         expect(receipt.boxItemCount == 1U, "room box item count") &&
         expect(receipt.lineItemCount == 0U, "room line item count") &&
         expect(receipt.pointItemCount == 0U, "room point item count") &&
         expect(receipt.skippedDegenerateCount == 0U,
                "room skipped count") &&
         expect(receipt.segmentCount == 12U, "room segment count") &&
         expect(receipt.wireframeStatus ==
                    cr::CreativeDocumentWireframeStatus::Built,
                "room wireframe status") &&
         expect(receipt.segmentStatus ==
                    cr::CreativeDocumentWireframeSegmentStatus::Built,
                "room segment status") &&
         expect(receipt.status ==
                    "product_creative_wireframe_frame_built",
                "room status");
}

bool hiddenRoomBuildsNoSegmentsButCountsObject() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade;
  const cr::CreativeObjectId roomId = createRoom(facade);
  selectTarget(facade, roomId);
  const cr::CreativeFacadeMutationReceipt mutation =
      facade.toggleSelectedObjectVisibility();

  const iggy3d::ProductCreativeWireframeFrameReceipt receipt =
      iggy3d::routeProductCreativeWireframeFrame(baseRequest(window, facade));

  return expect(mutation.accepted && mutation.changed,
                "hidden setup mutation changed") &&
         expect(receipt.sourceAvailable, "hidden source available") &&
         expect(receipt.objectCount == 1U, "hidden object count") &&
         expect(receipt.visibleObjectCount == 0U,
                "hidden visible object count") &&
         expect(receipt.itemCount == 0U, "hidden item count") &&
         expect(receipt.segmentCount == 0U, "hidden segment count") &&
         expect(receipt.wireframeStatus ==
                    cr::CreativeDocumentWireframeStatus::NoVisibleItems,
                "hidden wireframe status") &&
         expect(receipt.segmentStatus ==
                    cr::CreativeDocumentWireframeSegmentStatus::EmptySource,
                "hidden segment status") &&
         expect(receipt.status ==
                    "product_creative_wireframe_frame_no_segments",
                "hidden status");
}

bool createRoomCommandThenWireframeBuildsSegments() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade;
  iggy3d::ProductCreativeUiInputFrameReceipt input;
  input.consumed = true;
  input.enabled = true;
  input.semanticId = "creative.row.tools.create_room";

  const iggy3d::ProductCreativeUiCommandFrameReceipt command =
      iggy3d::routeProductCreativeUiCommandFrame(
          iggy3d::ProductCreativeUiCommandFrameRequest{&facade, input});
  const iggy3d::ProductCreativeWireframeFrameReceipt receipt =
      iggy3d::routeProductCreativeWireframeFrame(baseRequest(window, facade));

  return expect(command.accepted && command.changed,
                "create command accepted") &&
         expect(command.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::CreateRoom,
                "create command kind") &&
         expect(facade.document().objectCount() == 1U,
                "create command object count") &&
         expect(receipt.objectCount == 1U, "command wireframe object count") &&
         expect(receipt.segmentCount == 12U,
                "command wireframe segment count") &&
         expect(receipt.status ==
                    "product_creative_wireframe_frame_built",
                "command wireframe status");
}

bool defaultWindowReceiptFieldsAreNotRequested() {
  const iggy3d::ProductAppWindowState window;
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expectReceiptField(receipt,
                            "creative_wireframe_requested",
                            "false",
                            "default requested") &&
         expectReceiptField(receipt,
                            "creative_wireframe_active",
                            "false",
                            "default active") &&
         expectReceiptField(receipt,
                            "creative_wireframe_facade_available",
                            "false",
                            "default facade") &&
         expectReceiptField(receipt,
                            "creative_wireframe_document_available",
                            "false",
                            "default document") &&
         expectReceiptField(receipt,
                            "creative_wireframe_source_available",
                            "false",
                            "default source") &&
         expectReceiptField(receipt,
                            "creative_wireframe_object_count",
                            "0",
                            "default object count") &&
         expectReceiptField(receipt,
                            "creative_wireframe_visible_object_count",
                            "0",
                            "default visible count") &&
         expectReceiptField(receipt,
                            "creative_wireframe_item_count",
                            "0",
                            "default item count") &&
         expectReceiptField(receipt,
                            "creative_wireframe_segment_count",
                            "0",
                            "default segment count") &&
         expectReceiptField(receipt,
                            "creative_wireframe_box_item_count",
                            "0",
                            "default box count") &&
         expectReceiptField(receipt,
                            "creative_wireframe_line_item_count",
                            "0",
                            "default line count") &&
         expectReceiptField(receipt,
                            "creative_wireframe_point_item_count",
                            "0",
                            "default point count") &&
         expectReceiptField(receipt,
                            "creative_wireframe_skipped_degenerate_count",
                            "0",
                            "default skipped count") &&
         expectReceiptField(receipt,
                            "creative_wireframe_status",
                            "creative_wireframe_frame_not_requested",
                            "default status") &&
         expectReceiptField(receipt,
                            "creative_wireframe_reason_code",
                            "creative_wireframe_frame_not_requested",
                            "default reason") &&
         expectReceiptField(receipt,
                            "creative_wireframe_wireframe_status",
                            "Unknown",
                            "default wireframe status") &&
         expectReceiptField(receipt,
                            "creative_wireframe_wireframe_reason_code",
                            "none",
                            "default wireframe reason") &&
         expectReceiptField(receipt,
                            "creative_wireframe_segment_status",
                            "Unknown",
                            "default segment status") &&
         expectReceiptField(receipt,
                            "creative_wireframe_segment_reason_code",
                            "none",
                            "default segment reason");
}

bool recorderCopiesRoomReceiptFields() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade;
  (void)createRoom(facade);
  const iggy3d::ProductCreativeWireframeFrameReceipt frameReceipt =
      iggy3d::routeProductCreativeWireframeFrame(baseRequest(window, facade));

  iggy3d::ProductAppWindowState receiptWindow;
  iggy3d::recordProductCreativeWireframeFrame(receiptWindow, frameReceipt);
  const iggy3d::RenderReceipt receipt = receiptFor(receiptWindow);

  return expectReceiptField(receipt,
                            "creative_wireframe_requested",
                            "true",
                            "room receipt requested") &&
         expectReceiptField(receipt,
                            "creative_wireframe_active",
                            "true",
                            "room receipt active") &&
         expectReceiptField(receipt,
                            "creative_wireframe_facade_available",
                            "true",
                            "room receipt facade") &&
         expectReceiptField(receipt,
                            "creative_wireframe_document_available",
                            "true",
                            "room receipt document") &&
         expectReceiptField(receipt,
                            "creative_wireframe_source_available",
                            "true",
                            "room receipt source") &&
         expectReceiptField(receipt,
                            "creative_wireframe_object_count",
                            "1",
                            "room receipt object count") &&
         expectReceiptField(receipt,
                            "creative_wireframe_visible_object_count",
                            "1",
                            "room receipt visible count") &&
         expectReceiptField(receipt,
                            "creative_wireframe_item_count",
                            "1",
                            "room receipt item count") &&
         expectReceiptField(receipt,
                            "creative_wireframe_segment_count",
                            "12",
                            "room receipt segment count") &&
         expectReceiptField(receipt,
                            "creative_wireframe_box_item_count",
                            "1",
                            "room receipt box count") &&
         expectReceiptField(receipt,
                            "creative_wireframe_status",
                            "product_creative_wireframe_frame_built",
                            "room receipt status") &&
         expectReceiptField(receipt,
                            "creative_wireframe_reason_code",
                            "product_creative_wireframe_frame_built",
                            "room receipt reason") &&
         expectReceiptField(receipt,
                            "creative_wireframe_wireframe_status",
                            "Built",
                            "room receipt wireframe status") &&
         expectReceiptField(receipt,
                            "creative_wireframe_wireframe_reason_code",
                            "creative_document_wireframe_built",
                            "room receipt wireframe reason") &&
         expectReceiptField(receipt,
                            "creative_wireframe_segment_status",
                            "Built",
                            "room receipt segment status") &&
         expectReceiptField(receipt,
                            "creative_wireframe_segment_reason_code",
                            "creative_document_wireframe_segments_built",
                            "room receipt segment reason");
}

bool recorderPreservesExistingFields() {
  iggy3d::ProductAppWindowState window;
  window.status = "window_before";
  window.creativeUiInputRequested = true;
  window.creativeUiInputStatus = "input_before";
  window.creativeUiCommandRequested = true;
  window.creativeUiCommandStatus = "command_before";
  window.creativeViewportPickRequested = true;
  window.creativeViewportPickStatus = "pick_before";
  window.activeCreativeSaveStatus = "save_before";
  window.productVulkanMenuUiReady = true;
  window.productVulkanMenuUiStatus = "vulkan_before";

  iggy3d::ProductCreativeWireframeFrameReceipt receipt;
  receipt.requested = true;
  receipt.status = "product_creative_wireframe_frame_built";
  receipt.reasonCode = receipt.status;
  receipt.segmentCount = 12;
  iggy3d::recordProductCreativeWireframeFrame(window, receipt);

  return expect(window.status == "window_before", "window status kept") &&
         expect(window.creativeUiInputRequested,
                "creative input requested kept") &&
         expect(window.creativeUiInputStatus == "input_before",
                "creative input status kept") &&
         expect(window.creativeUiCommandRequested,
                "creative command requested kept") &&
         expect(window.creativeUiCommandStatus == "command_before",
                "creative command status kept") &&
         expect(window.creativeViewportPickRequested,
                "viewport pick requested kept") &&
         expect(window.creativeViewportPickStatus == "pick_before",
                "viewport pick status kept") &&
         expect(window.activeCreativeSaveStatus == "save_before",
                "creative save status kept") &&
         expect(window.productVulkanMenuUiReady, "vulkan ready kept") &&
         expect(window.productVulkanMenuUiStatus == "vulkan_before",
                "vulkan status kept");
}

}  // namespace

int main() {
  const bool ok =
      activeRuleUsesOnlyInteractionMode() &&
      inactiveWindowDoesNotBuild() &&
      nullFacadeFailsClosed() &&
      emptyCreativeDocumentBuildsZeroSegments() &&
      oneRoomBuildsTwelveSegments() &&
      hiddenRoomBuildsNoSegmentsButCountsObject() &&
      createRoomCommandThenWireframeBuildsSegments() &&
      defaultWindowReceiptFieldsAreNotRequested() &&
      recorderCopiesRoomReceiptFields() &&
      recorderPreservesExistingFields();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
