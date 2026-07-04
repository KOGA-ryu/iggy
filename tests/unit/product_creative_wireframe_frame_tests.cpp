#include "app/iggy3d/window/CreativeWireframeFrame.hpp"

#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/view/CreativeWireframeDebugLines.hpp"
#include "app/iggy3d/window/CreativeUiCommandFrame.hpp"
#include "app/iggy3d/window/FramePresenter.hpp"
#include "projection/scene/SceneItem.hpp"
#include "render/vulkan/BufferImageResources.hpp"

#include <cstdint>
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
  window.activeCreativeSaveId = "creative_save";
  window.activeCreativeWorldId = "world_001";
  window.activeCreativeDocumentId = 42U;
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

iggy3d::SceneRoomProjection packageRoomProjection() {
  iggy3d::SceneRoomMeshItem prop;
  prop.id = "w8_h_prop";
  prop.role = "prop";
  prop.materialId = "w8_h_debug_material";
  prop.position = {0.0F, 0.5F, 0.0F};
  prop.size = {1.0F, 1.0F, 1.0F};

  iggy3d::SceneRoomProjection room;
  room.loaded = true;
  room.assetId = "w8_h_package_room";
  room.version = 1;
  room.staticMeshCount = 1U;
  room.meshes.push_back(prop);
  return room;
}

bool activeRuleUsesCreativeDocumentIdentity() {
  iggy3d::ProductAppWindowState player;
  player.interactionMode = iggy3d::ProductInteractionMode::Player;
  iggy3d::ProductAppWindowState legacyCreative;
  legacyCreative.interactionMode = iggy3d::ProductInteractionMode::Creative;
  iggy3d::ProductAppWindowState documentCreative = creativeWindow();

  return expect(!iggy3d::productCreativeWireframeFrameActiveForWindow(player),
                "player wireframe inactive") &&
         expect(!iggy3d::productCreativeWireframeFrameActiveForWindow(
                    legacyCreative),
                "legacy creative wireframe inactive") &&
         expect(iggy3d::productCreativeWireframeFrameActiveForWindow(
                    documentCreative),
                "document creative wireframe active");
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
         expect(receipt.debugLineRequested, "empty debug line requested") &&
         expect(receipt.debugLineSourceAvailable,
                "empty debug line source") &&
         expect(receipt.debugLineInputSegmentCount == 0U,
                "empty debug line input segments") &&
         expect(receipt.debugLineCount == 0U,
                "empty debug line count") &&
         expect(receipt.debugLineSkippedDegenerateCount == 0U,
                "empty debug line skipped count") &&
         expect(receipt.debugLineStatus ==
                    iggy3d::ProductCreativeWireframeDebugLineStatus::NoLines,
                "empty debug line status") &&
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
         expect(receipt.debugLineRequested, "room debug line requested") &&
         expect(receipt.debugLineSourceAvailable,
                "room debug line source") &&
         expect(receipt.debugLineInputSegmentCount == 12U,
                "room debug line input segments") &&
         expect(receipt.debugLineCount == 12U, "room debug line count") &&
         expect(receipt.debugLineSkippedDegenerateCount == 0U,
                "room debug line skipped count") &&
         expect(receipt.debugLineStatus ==
                    iggy3d::ProductCreativeWireframeDebugLineStatus::Built,
                "room debug line status") &&
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

bool buildResultKeepsDebugLineList() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade;
  const cr::CreativeObjectId roomId = createRoom(facade);

  const iggy3d::ProductCreativeWireframeFrameBuildResult result =
      iggy3d::buildProductCreativeWireframeFrame(baseRequest(window, facade));

  return expect(result.receipt.status ==
                    "product_creative_wireframe_frame_built",
                "build result status") &&
         expect(result.receipt.debugLineCount == 12U,
                "build result debug line count") &&
         expect(result.debugLineList.lines.size() == 12U,
                "build result line list size") &&
         expect(result.debugLineList.lines.front().objectId == roomId,
                "build result line object id");
}

bool presenterRenderFrameCarriesDebugLines() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade;
  const cr::CreativeObjectId roomId = createRoom(facade);
  const iggy3d::ProductCreativeWireframeFrameBuildResult result =
      iggy3d::buildProductCreativeWireframeFrame(baseRequest(window, facade));

  const iggy3d::ProductCreativeWireframeDebugRenderFrame renderFrame =
      iggy3d::buildProductCreativeWireframeDebugRenderFrame(
          &result.debugLineList);

  return expect(renderFrame.frame.available, "render frame available") &&
         expect(renderFrame.frame.visible, "render frame visible") &&
         expect(renderFrame.frame.lineCount == 12U,
                "render frame line count") &&
         expect(renderFrame.frame.lines == renderFrame.lines.data(),
                "render frame pointer owned") &&
         expect(renderFrame.lines.front().objectId ==
                    static_cast<std::uint64_t>(roomId),
                "render line object id") &&
         expect(renderFrame.lines.front().thickness == 1.0F,
                "render line thickness");
}

bool visibleRoomWireframeReachesVulkanCpuGeometry() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade;
  const cr::CreativeObjectId roomId = createRoom(facade);
  const iggy3d::ProductCreativeWireframeFrameBuildResult frameResult =
      iggy3d::buildProductCreativeWireframeFrame(baseRequest(window, facade));
  const iggy3d::ProductCreativeWireframeDebugRenderFrame renderFrame =
      iggy3d::buildProductCreativeWireframeDebugRenderFrame(
          &frameResult.debugLineList);
  const iggy3d::SceneRoomProjection packageRoom = packageRoomProjection();
  const iggy3d::vulkan::RoomMeshCpuGeometry geometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(packageRoom,
                                               &renderFrame.frame);

  return expect(roomId != cr::kInvalidObjectId, "composition room created") &&
         expect(frameResult.receipt.objectCount == 1U,
                "composition object count") &&
         expect(frameResult.receipt.debugLineCount == 12U,
                "composition debug line count") &&
         expect(renderFrame.frame.available,
                "composition render debug available") &&
         expect(renderFrame.frame.visible,
                "composition render debug visible") &&
         expect(renderFrame.frame.lineCount == 12U,
                "composition render line count") &&
         expect(geometry.ready, "composition geometry ready") &&
         expect(geometry.creativeWireframeDebugLineInputCount == 12U,
                "composition geometry input count") &&
         expect(geometry.creativeWireframeDebugGeometryDrawCount == 12U,
                "composition geometry draw count") &&
         expect(geometry.creativeWireframeDebugGeometrySkippedCount == 0U,
                "composition geometry skipped count") &&
         expect(geometry.creativeWireframeDebugGeometryReasonCode ==
                    "vulkan_creative_wireframe_debug_geometry_built",
                "composition geometry reason") &&
         expect(geometry.indexedDraws.size() >= 13U,
                "composition appended debug draw ranges");
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
         expect(receipt.debugLineRequested, "hidden debug line requested") &&
         expect(receipt.debugLineSourceAvailable,
                "hidden debug line source") &&
         expect(receipt.debugLineInputSegmentCount == 0U,
                "hidden debug line input segments") &&
         expect(receipt.debugLineCount == 0U,
                "hidden debug line count") &&
         expect(receipt.debugLineStatus ==
                    iggy3d::ProductCreativeWireframeDebugLineStatus::NoLines,
                "hidden debug line status") &&
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

bool hiddenRoomWireframeDoesNotAppendVulkanDebugGeometry() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade;
  const cr::CreativeObjectId roomId = createRoom(facade);
  selectTarget(facade, roomId);
  const cr::CreativeFacadeMutationReceipt mutation =
      facade.toggleSelectedObjectVisibility();
  const iggy3d::ProductCreativeWireframeFrameBuildResult frameResult =
      iggy3d::buildProductCreativeWireframeFrame(baseRequest(window, facade));
  const iggy3d::ProductCreativeWireframeDebugRenderFrame renderFrame =
      iggy3d::buildProductCreativeWireframeDebugRenderFrame(
          &frameResult.debugLineList);
  const iggy3d::SceneRoomProjection packageRoom = packageRoomProjection();
  const iggy3d::vulkan::RoomMeshCpuGeometry geometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(packageRoom,
                                               &renderFrame.frame);

  return expect(mutation.accepted && mutation.changed,
                "hidden composition mutation changed") &&
         expect(facade.document().objectCount() == 1U,
                "hidden composition object remains") &&
         expect(frameResult.receipt.objectCount == 1U,
                "hidden composition receipt object count") &&
         expect(frameResult.receipt.debugLineCount == 0U,
                "hidden composition debug line count") &&
         expect(renderFrame.frame.available,
                "hidden composition render debug available") &&
         expect(!renderFrame.frame.visible,
                "hidden composition render debug hidden") &&
         expect(renderFrame.frame.lineCount == 0U,
                "hidden composition render line count") &&
         expect(geometry.ready, "hidden composition base geometry ready") &&
         expect(geometry.creativeWireframeDebugLineInputCount == 0U,
                "hidden composition geometry input count") &&
         expect(geometry.creativeWireframeDebugGeometryDrawCount == 0U,
                "hidden composition geometry draw count") &&
         expect(geometry.creativeWireframeDebugGeometrySkippedCount == 0U,
                "hidden composition geometry skipped count") &&
         expect(geometry.creativeWireframeDebugGeometryReasonCode ==
                    "vulkan_creative_wireframe_debug_geometry_no_lines",
                "hidden composition geometry reason");
}

bool missingCreativeDebugSourceKeepsVulkanGeometryNotRequested() {
  const iggy3d::ProductCreativeWireframeDebugRenderFrame renderFrame =
      iggy3d::buildProductCreativeWireframeDebugRenderFrame(nullptr);
  const iggy3d::SceneRoomProjection packageRoom = packageRoomProjection();
  const iggy3d::vulkan::RoomMeshCpuGeometry geometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(packageRoom,
                                               &renderFrame.frame);

  return expect(!renderFrame.frame.available,
                "missing source render frame unavailable") &&
         expect(!renderFrame.frame.visible,
                "missing source render frame hidden") &&
         expect(renderFrame.frame.lineCount == 0U,
                "missing source render line count") &&
         expect(geometry.ready, "missing source base geometry ready") &&
         expect(geometry.creativeWireframeDebugLineInputCount == 0U,
                "missing source geometry input count") &&
         expect(geometry.creativeWireframeDebugGeometryDrawCount == 0U,
                "missing source geometry draw count") &&
         expect(geometry.creativeWireframeDebugGeometryReasonCode ==
                    "vulkan_creative_wireframe_debug_geometry_not_requested",
                "missing source geometry reason");
}

bool creativeDebugGeometrySignatureTracksVisibilityState() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade;
  const cr::CreativeObjectId roomId = createRoom(facade);
  const iggy3d::ProductCreativeWireframeFrameBuildResult visibleFrame =
      iggy3d::buildProductCreativeWireframeFrame(baseRequest(window, facade));
  const iggy3d::ProductCreativeWireframeDebugRenderFrame visibleRenderFrame =
      iggy3d::buildProductCreativeWireframeDebugRenderFrame(
          &visibleFrame.debugLineList);
  selectTarget(facade, roomId);
  static_cast<void>(facade.toggleSelectedObjectVisibility());
  const iggy3d::ProductCreativeWireframeFrameBuildResult hiddenFrame =
      iggy3d::buildProductCreativeWireframeFrame(baseRequest(window, facade));
  const iggy3d::ProductCreativeWireframeDebugRenderFrame hiddenRenderFrame =
      iggy3d::buildProductCreativeWireframeDebugRenderFrame(
          &hiddenFrame.debugLineList);
  const iggy3d::SceneRoomProjection packageRoom = packageRoomProjection();
  const iggy3d::vulkan::RoomMeshCpuGeometry visibleGeometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(packageRoom,
                                               &visibleRenderFrame.frame);
  const iggy3d::vulkan::RoomMeshCpuGeometry hiddenGeometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(packageRoom,
                                               &hiddenRenderFrame.frame);

  return expect(visibleGeometry.sourceCreativeWireframeDebugSignature != 0U,
                "visible signature present") &&
         expect(hiddenGeometry.sourceCreativeWireframeDebugSignature != 0U,
                "hidden signature present") &&
         expect(visibleGeometry.sourceCreativeWireframeDebugSignature !=
                    hiddenGeometry.sourceCreativeWireframeDebugSignature,
                "visibility changes debug geometry signature");
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
         expect(receipt.debugLineCount == 12U,
                "command wireframe debug line count") &&
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
                            "default segment reason") &&
         expectReceiptField(receipt,
                            "creative_wireframe_debug_line_requested",
                            "false",
                            "default debug line requested") &&
         expectReceiptField(receipt,
                            "creative_wireframe_debug_line_source_available",
                            "false",
                            "default debug line source") &&
         expectReceiptField(receipt,
                            "creative_wireframe_debug_line_input_segment_count",
                            "0",
                            "default debug line input segments") &&
         expectReceiptField(receipt,
                            "creative_wireframe_debug_line_count",
                            "0",
                            "default debug line count") &&
         expectReceiptField(receipt,
                            "creative_wireframe_debug_line_skipped_degenerate_count",
                            "0",
                            "default debug line skipped count") &&
         expectReceiptField(receipt,
                            "creative_wireframe_debug_line_status",
                            "Unknown",
                            "default debug line status") &&
         expectReceiptField(receipt,
                            "creative_wireframe_debug_line_reason_code",
                            "none",
                            "default debug line reason");
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
                            "room receipt segment reason") &&
         expectReceiptField(receipt,
                            "creative_wireframe_debug_line_requested",
                            "true",
                            "room receipt debug line requested") &&
         expectReceiptField(receipt,
                            "creative_wireframe_debug_line_source_available",
                            "true",
                            "room receipt debug line source") &&
         expectReceiptField(receipt,
                            "creative_wireframe_debug_line_input_segment_count",
                            "12",
                            "room receipt debug line input segments") &&
         expectReceiptField(receipt,
                            "creative_wireframe_debug_line_count",
                            "12",
                            "room receipt debug line count") &&
         expectReceiptField(receipt,
                            "creative_wireframe_debug_line_skipped_degenerate_count",
                            "0",
                            "room receipt debug line skipped count") &&
         expectReceiptField(receipt,
                            "creative_wireframe_debug_line_status",
                            "Built",
                            "room receipt debug line status") &&
         expectReceiptField(receipt,
                            "creative_wireframe_debug_line_reason_code",
                            "product_creative_wireframe_debug_lines_built",
                            "room receipt debug line reason");
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

bool startupFrameMeasurementFieldsAreReceiptOnlyScalars() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  window.startupCreativeUiFirstFrameMeasured = true;
  window.startupCreativeUiFirstFrameMicroseconds = 17;
  window.startupCreativeUiFirstFrameStatus =
      "product_creative_ui_frame_ready";
  window.startupCreativeWireframeFirstFrameMeasured = true;
  window.startupCreativeWireframeFirstFrameMicroseconds = 23;
  window.startupCreativeWireframeFirstFrameStatus =
      "product_creative_wireframe_frame_built";
  window.startupVulkanRendererInitMeasured = true;
  window.startupVulkanRendererInitMicroseconds = 31;
  window.startupVulkanRendererInitStatus =
      "startup_vulkan_renderer_init_ready";
  window.startupVulkanFirstSubmitMeasured = true;
  window.startupVulkanFirstSubmitMicroseconds = 43;
  window.startupVulkanFirstSubmitStatus = "renderer_ok";

  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expectReceiptField(receipt,
                            "startup_creative_ui_first_frame_measured",
                            "true",
                            "startup ui frame measured") &&
         expectReceiptField(receipt,
                            "startup_creative_ui_first_frame_us",
                            "17",
                            "startup ui frame us") &&
         expectReceiptField(receipt,
                            "startup_creative_wireframe_first_frame_measured",
                            "true",
                            "startup wireframe measured") &&
         expectReceiptField(receipt,
                            "startup_creative_wireframe_first_frame_status",
                            "product_creative_wireframe_frame_built",
                            "startup wireframe status") &&
         expectReceiptField(receipt,
                            "startup_vulkan_renderer_init_measured",
                            "true",
                            "startup renderer measured") &&
         expectReceiptField(receipt,
                            "startup_vulkan_renderer_init_us",
                            "31",
                            "startup renderer us") &&
         expectReceiptField(receipt,
                            "startup_vulkan_first_submit_measured",
                            "true",
                            "startup submit measured") &&
         expectReceiptField(receipt,
                            "startup_vulkan_first_submit_status",
                            "renderer_ok",
                            "startup submit status");
}

}  // namespace

int main() {
  const bool ok =
      activeRuleUsesCreativeDocumentIdentity() &&
      inactiveWindowDoesNotBuild() &&
      nullFacadeFailsClosed() &&
      emptyCreativeDocumentBuildsZeroSegments() &&
      oneRoomBuildsTwelveSegments() &&
      buildResultKeepsDebugLineList() &&
      presenterRenderFrameCarriesDebugLines() &&
      visibleRoomWireframeReachesVulkanCpuGeometry() &&
      hiddenRoomBuildsNoSegmentsButCountsObject() &&
      hiddenRoomWireframeDoesNotAppendVulkanDebugGeometry() &&
      missingCreativeDebugSourceKeepsVulkanGeometryNotRequested() &&
      creativeDebugGeometrySignatureTracksVisibilityState() &&
      createRoomCommandThenWireframeBuildsSegments() &&
      defaultWindowReceiptFieldsAreNotRequested() &&
      recorderCopiesRoomReceiptFields() &&
      recorderPreservesExistingFields() &&
      startupFrameMeasurementFieldsAreReceiptOnlyScalars();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
