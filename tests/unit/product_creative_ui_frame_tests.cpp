#include "app/iggy3d/menu/CreativeUiFrame.hpp"

#include "app/iggy3d/creative/Facade.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
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

bool expectReceiptCount(const iggy3d::RenderReceipt& receipt,
                        std::string_view key,
                        std::uint64_t value,
                        std::string_view message) {
  const std::string text = std::to_string(value);
  return expectReceiptField(receipt,
                            key,
                            std::string_view(text.data(), text.size()),
                            message);
}

void populateSelectedFacade(cr::Facade& facade) {
  // Move selects like Select until the drag slice (TV1-F/G) lands.
  facade.reset();
  static_cast<void>(facade.setActiveTool(cr::Tool::Move));

  cr::CreativeToolInputPacket input;
  input.kind = cr::CreativeToolInputKind::PointerPress;
  input.pointer.button = cr::CreativeToolPointerButton::Primary;
  input.pointer.target.value = 88;
  static_cast<void>(facade.dispatchToolInput(input));
}

void prepopulateCreativeProjection(iggy3d::ProductAppWindowState& window) {
  window.creativeUiProjectionRequested = true;
  window.creativeUiProjectionReady = true;
  window.creativeUiProjectionPartial = true;
  window.creativeUiProjectionStatus = "stale_ready";
  window.creativeUiProjectionReasonCode = "stale_ready";
  window.creativeUiProjectionUsedModel = true;
  window.creativeUiProjectionUsedFacade = true;
  window.creativeUiProjectionVirtualWidth = 640;
  window.creativeUiProjectionVirtualHeight = 360;
  window.creativeUiProjectionTheme = "journal";
  window.creativeUiProjectionPanelCount = 11;
  window.creativeUiProjectionModelRowCount = 12;
  window.creativeUiProjectionPrimitiveCount = 13;
  window.creativeUiProjectionTextCount = 14;
  window.creativeUiProjectionRectCount = 15;
  window.creativeUiProjectionRowCount = 16;
  window.creativeUiProjectionDisabledRowCount = 17;
  window.creativeUiProjectionHitRegionCount = 18;
}

void markCreativeDocumentWindow(iggy3d::ProductAppWindowState& window) {
  window.interactionMode = iggy3d::ProductInteractionMode::Creative;
  window.activeCreativeSaveId = "creative_save";
  window.activeCreativeWorldId = "world_001";
  window.activeCreativeDocumentId = 42U;
}

void clearCreativeDocumentIdentity(iggy3d::ProductAppWindowState& window) {
  window.activeCreativeSaveId = "none";
  window.activeCreativeWorldId = "none";
  window.activeCreativeDocumentId = 0U;
}

bool activeRuleUsesCreativeDocumentIdentity() {
  iggy3d::ProductAppWindowState window;
  const bool playerActive = iggy3d::productCreativeUiActiveForWindow(window);
  window.interactionMode = iggy3d::ProductInteractionMode::Creative;
  const bool legacyCreativeActive =
      iggy3d::productCreativeUiActiveForWindow(window);
  markCreativeDocumentWindow(window);
  const bool documentCreativeActive =
      iggy3d::productCreativeUiActiveForWindow(window);

  return expect(!playerActive, "player inactive") &&
         expect(!legacyCreativeActive, "legacy creative inactive") &&
         expect(documentCreativeActive, "document creative active");
}

bool nullWindowReturnsWindowMissing() {
  const iggy3d::ProductCreativeUiFrame frame =
      iggy3d::buildProductCreativeUiFrame({});

  return expect(!frame.receipt.requested, "null not requested") &&
         expect(!frame.receipt.active, "null not active") &&
         expect(!frame.receipt.projected, "null not projected") &&
         expect(!frame.receipt.recorded, "null not recorded") &&
         expect(!frame.receipt.ready, "null not ready") &&
         expect(frame.receipt.status ==
                    "product_creative_ui_frame_window_missing",
                "null status") &&
         expect(frame.receipt.reasonCode ==
                    "product_creative_ui_frame_window_missing",
                "null reason") &&
         expect(frame.receipt.primitiveCount == 0U, "null primitive count") &&
         expect(frame.receipt.hitRegionCount == 0U, "null hit count");
}

bool inactiveWindowRecordsInactiveProjection() {
  iggy3d::ProductAppWindowState window;
  prepopulateCreativeProjection(window);

  iggy3d::ProductCreativeUiFrameRequest request;
  request.window = &window;
  request.virtualWidth = 1440;
  request.virtualHeight = 900;
  request.theme = iggy3d::ProductUiThemeId::Journal;
  const iggy3d::ProductCreativeUiFrame frame =
      iggy3d::buildProductCreativeUiFrame(request);
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expect(!frame.receipt.requested, "inactive frame not requested") &&
         expect(!frame.receipt.active, "inactive frame not active") &&
         expect(!frame.receipt.projected, "inactive not projected") &&
         expect(frame.receipt.recorded, "inactive recorded") &&
         expect(!frame.receipt.ready, "inactive not ready") &&
         expect(frame.receipt.status == "product_creative_ui_frame_inactive",
                "inactive frame status") &&
         expect(!window.creativeUiProjectionRequested,
                "inactive projection not requested") &&
         expect(!window.creativeUiProjectionReady,
                "inactive projection not ready") &&
         expect(window.creativeUiProjectionStatus ==
                    "product_creative_ui_frame_inactive",
                "inactive window status") &&
         expect(window.creativeUiProjectionVirtualWidth == 1440U,
                "inactive width copied") &&
         expect(window.creativeUiProjectionVirtualHeight == 900U,
                "inactive height copied") &&
         expect(window.creativeUiProjectionTheme == "journal",
                "inactive theme copied") &&
         expect(window.creativeUiProjectionPanelCount == 0U,
                "inactive panel count cleared") &&
         expect(window.creativeUiProjectionPrimitiveCount == 0U,
                "inactive primitive count cleared") &&
         expect(window.creativeUiProjectionTextCount == 0U,
                "inactive text count cleared") &&
         expect(window.creativeUiProjectionRectCount == 0U,
                "inactive rect count cleared") &&
         expect(window.creativeUiProjectionRowCount == 0U,
                "inactive row count cleared") &&
         expect(window.creativeUiProjectionHitRegionCount == 0U,
                "inactive hit count cleared") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_status",
                            "product_creative_ui_frame_inactive",
                            "inactive receipt status") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_requested",
                            "false",
                            "inactive receipt requested") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_theme",
                            "journal",
                            "inactive receipt theme") &&
         expectReceiptCount(receipt,
                            "creative_ui_projection_virtual_width",
                            1440,
                            "inactive receipt width") &&
         expectReceiptCount(receipt,
                            "creative_ui_projection_primitive_count",
                            0,
                            "inactive receipt primitive count");
}

bool creativeWindowMissingFacadeRecordsFailure() {
  iggy3d::ProductAppWindowState window;
  markCreativeDocumentWindow(window);
  prepopulateCreativeProjection(window);

  iggy3d::ProductCreativeUiFrameRequest request;
  request.window = &window;
  request.virtualWidth = 1024;
  request.virtualHeight = 768;
  const iggy3d::ProductCreativeUiFrame frame =
      iggy3d::buildProductCreativeUiFrame(request);
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expect(frame.receipt.requested, "missing facade requested") &&
         expect(frame.receipt.active, "missing facade active") &&
         expect(!frame.receipt.projected, "missing facade not projected") &&
         expect(frame.receipt.recorded, "missing facade recorded") &&
         expect(!frame.receipt.ready, "missing facade not ready") &&
         expect(frame.receipt.status ==
                    "product_creative_ui_frame_facade_missing",
                "missing facade frame status") &&
         expect(window.creativeUiProjectionRequested,
                "missing facade projection requested") &&
         expect(!window.creativeUiProjectionReady,
                "missing facade projection not ready") &&
         expect(!window.creativeUiProjectionUsedFacade,
                "missing facade used facade false") &&
         expect(window.creativeUiProjectionStatus ==
                    "product_creative_ui_frame_facade_missing",
                "missing facade window status") &&
         expect(window.creativeUiProjectionPrimitiveCount == 0U,
                "missing facade primitive count zero") &&
         expect(window.creativeUiProjectionHitRegionCount == 0U,
                "missing facade hit count zero") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_status",
                            "product_creative_ui_frame_facade_missing",
                            "missing facade receipt status") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_requested",
                            "true",
                            "missing facade receipt requested") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_ready",
                            "false",
                            "missing facade receipt ready") &&
         expectReceiptCount(receipt,
                            "creative_ui_projection_primitive_count",
                            0,
                            "missing facade receipt primitive count");
}

bool creativeWindowWithFacadeProjectsAndRecords() {
  iggy3d::ProductAppWindowState window;
  markCreativeDocumentWindow(window);
  cr::Facade facade;
  populateSelectedFacade(facade);

  iggy3d::ProductCreativeUiFrameRequest request;
  request.window = &window;
  request.facade = &facade;
  const iggy3d::ProductCreativeUiFrame frame =
      iggy3d::buildProductCreativeUiFrame(request);
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expect(frame.receipt.requested, "active requested") &&
         expect(frame.receipt.active, "active frame active") &&
         expect(frame.receipt.projected, "active projected") &&
         expect(frame.receipt.recorded, "active recorded") &&
         expect(frame.receipt.ready, "active ready") &&
         expect(window.creativeUiProjectionReady, "window ready") &&
         expect(window.creativeUiProjectionUsedFacade, "window used facade") &&
         expect(!window.creativeUiProjectionUsedModel, "window model unused") &&
         expect(window.creativeUiProjectionPanelCount > 0U,
                "panel count nonzero") &&
         expect(window.creativeUiProjectionModelRowCount > 0U,
                "model row count nonzero") &&
         expect(window.creativeUiProjectionPrimitiveCount > 0U,
                "primitive count nonzero") &&
         expect(window.creativeUiProjectionTextCount > 0U,
                "text count nonzero") &&
         expect(window.creativeUiProjectionRectCount > 0U,
                "rect count nonzero") &&
         expect(window.creativeUiProjectionRowCount > 0U,
                "row count nonzero") &&
         expect(window.creativeUiProjectionHitRegionCount > 0U,
                "hit count nonzero") &&
         expect(window.creativeUiProjectionHitRegionCount ==
                    window.creativeUiProjectionRowCount,
                "hit count mirrors row count") &&
         expect(frame.receipt.primitiveCount ==
                    window.creativeUiProjectionPrimitiveCount,
                "frame primitive count copied") &&
         expect(frame.receipt.textCount == window.creativeUiProjectionTextCount,
                "frame text count copied") &&
         expect(frame.receipt.rectCount == window.creativeUiProjectionRectCount,
                "frame rect count copied") &&
         expect(frame.receipt.rowCount == window.creativeUiProjectionRowCount,
                "frame row count copied") &&
         expect(frame.receipt.hitRegionCount ==
                    window.creativeUiProjectionHitRegionCount,
                "frame hit count copied") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_used_facade",
                            "true",
                            "receipt used facade") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_ready",
                            "true",
                            "receipt ready");
}

bool activeThenInactiveClearsPriorReadyProjection() {
  iggy3d::ProductAppWindowState window;
  markCreativeDocumentWindow(window);
  cr::Facade facade;
  facade.reset();

  iggy3d::ProductCreativeUiFrameRequest request;
  request.window = &window;
  request.facade = &facade;
  const iggy3d::ProductCreativeUiFrame activeFrame =
      iggy3d::buildProductCreativeUiFrame(request);

  window.interactionMode = iggy3d::ProductInteractionMode::Player;
  clearCreativeDocumentIdentity(window);
  const iggy3d::ProductCreativeUiFrame inactiveFrame =
      iggy3d::buildProductCreativeUiFrame(request);

  return expect(activeFrame.receipt.ready, "first active ready") &&
         expect(window.creativeUiProjectionStatus ==
                    "product_creative_ui_frame_inactive",
                "cleared inactive status") &&
         expect(!window.creativeUiProjectionRequested,
                "cleared inactive requested") &&
         expect(!window.creativeUiProjectionReady, "cleared inactive ready") &&
         expect(!window.creativeUiProjectionUsedFacade,
                "cleared inactive facade") &&
         expect(window.creativeUiProjectionPrimitiveCount == 0U,
                "cleared primitive count") &&
         expect(window.creativeUiProjectionTextCount == 0U,
                "cleared text count") &&
         expect(window.creativeUiProjectionRectCount == 0U,
                "cleared rect count") &&
         expect(window.creativeUiProjectionRowCount == 0U,
                "cleared row count") &&
         expect(!inactiveFrame.receipt.projected,
                "inactive frame not projected") &&
         expect(inactiveFrame.receipt.recorded, "inactive frame recorded") &&
         expect(inactiveFrame.receipt.status ==
                    "product_creative_ui_frame_inactive",
                "inactive frame status");
}

bool productVulkanMenuUiFieldsAreUnchanged() {
  iggy3d::ProductAppWindowState window;
  markCreativeDocumentWindow(window);
  window.productVulkanMenuUiReady = true;
  window.productVulkanMenuUiPartial = true;
  window.productVulkanMenuUiStatus = "preexisting_ui_status";
  window.productVulkanMenuUiReasonCode = "preexisting_ui_reason";
  window.productVulkanMenuUiPrimitiveCount = 101;
  window.productVulkanMenuUiTextCount = 102;
  window.productVulkanMenuUiRectCount = 103;
  window.productVulkanMenuUiRowCount = 104;
  window.productVulkanMenuUiSelectedAction = "preexisting_action";

  cr::Facade facade;
  facade.reset();
  iggy3d::ProductCreativeUiFrameRequest request;
  request.window = &window;
  request.facade = &facade;
  static_cast<void>(iggy3d::buildProductCreativeUiFrame(request));

  return expect(window.productVulkanMenuUiReady,
                "vulkan ui ready unchanged") &&
         expect(window.productVulkanMenuUiPartial,
                "vulkan ui partial unchanged") &&
         expect(window.productVulkanMenuUiStatus == "preexisting_ui_status",
                "vulkan ui status unchanged") &&
         expect(window.productVulkanMenuUiReasonCode ==
                    "preexisting_ui_reason",
                "vulkan ui reason unchanged") &&
         expect(window.productVulkanMenuUiPrimitiveCount == 101U,
                "vulkan ui primitive count unchanged") &&
         expect(window.productVulkanMenuUiTextCount == 102U,
                "vulkan ui text count unchanged") &&
         expect(window.productVulkanMenuUiRectCount == 103U,
                "vulkan ui rect count unchanged") &&
         expect(window.productVulkanMenuUiRowCount == 104U,
                "vulkan ui row count unchanged") &&
         expect(window.productVulkanMenuUiSelectedAction ==
                    "preexisting_action",
                "vulkan ui action unchanged");
}

bool facadeStateIsNotMutatedByProjection() {
  iggy3d::ProductAppWindowState window;
  markCreativeDocumentWindow(window);
  cr::Facade facade;
  populateSelectedFacade(facade);

  const cr::Tool activeToolBefore = facade.toolState().activeTool;
  const cr::TargetRef selectedBefore =
      facade.selectionState().selectedTarget;

  iggy3d::ProductCreativeUiFrameRequest request;
  request.window = &window;
  request.facade = &facade;
  static_cast<void>(iggy3d::buildProductCreativeUiFrame(request));

  return expect(facade.toolState().activeTool == activeToolBefore,
                "facade active tool unchanged") &&
         expect(facade.selectionState().selectedTarget.value ==
                    selectedBefore.value,
                "facade selected target unchanged");
}

}  // namespace

int main() {
  const bool ok = activeRuleUsesCreativeDocumentIdentity() &&
                  nullWindowReturnsWindowMissing() &&
                  inactiveWindowRecordsInactiveProjection() &&
                  creativeWindowMissingFacadeRecordsFailure() &&
                  creativeWindowWithFacadeProjectsAndRecords() &&
                  activeThenInactiveClearsPriorReadyProjection() &&
                  productVulkanMenuUiFieldsAreUnchanged() &&
                  facadeStateIsNotMutatedByProjection();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
