#include "app/iggy3d/creative/ui/UiFrame.hpp"

#include "ProductReceiptTestSupport.hpp"

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/iggy3d/Operations.hpp"
#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"
#include "app/iggy3d/world/DefaultWorldTemplate.hpp"
#include "render/RenderDiagnostics.hpp"

#include <cstdlib>
#include <string>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

using iggy3d::test::expect;
using iggy3d::test::expectReceiptCount;
using iggy3d::test::expectReceiptField;
using iggy3d::test::receiptFor;

void markCreativeAppIdentity(cr::CreativeAppState& app) {
  app.identity.saveId = "creative_save";
  app.identity.worldId = "world_001";
  app.identity.documentId = 42U;
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
  window.creativeAuthoring.creativeUiProjection.requested = true;
  window.creativeAuthoring.creativeUiProjection.ready = true;
  window.creativeAuthoring.creativeUiProjection.partial = true;
  window.creativeAuthoring.creativeUiProjection.status = "stale_ready";
  window.creativeAuthoring.creativeUiProjection.reasonCode = "stale_ready";
  window.creativeAuthoring.creativeUiProjection.usedModel = true;
  window.creativeAuthoring.creativeUiProjection.usedFacade = true;
  window.creativeAuthoring.creativeUiProjection.virtualWidth = 640;
  window.creativeAuthoring.creativeUiProjection.virtualHeight = 360;
  window.creativeAuthoring.creativeUiProjection.theme = "journal";
  window.creativeAuthoring.creativeUiProjection.panelCount = 11;
  window.creativeAuthoring.creativeUiProjection.modelRowCount = 12;
  window.creativeAuthoring.creativeUiProjection.primitiveCount = 13;
  window.creativeAuthoring.creativeUiProjection.textCount = 14;
  window.creativeAuthoring.creativeUiProjection.rectCount = 15;
  window.creativeAuthoring.creativeUiProjection.rowCount = 16;
  window.creativeAuthoring.creativeUiProjection.disabledRowCount = 17;
  window.creativeAuthoring.creativeUiProjection.hitRegionCount = 18;
}

void markCreativeDocumentWindow(iggy3d::ProductAppWindowState& window) {
  window.inputDevice.interactionMode = iggy3d::ProductInteractionMode::Creative;
}

void clearCreativeDocumentIdentity(iggy3d::ProductAppWindowState& window) {
  window.inputDevice.interactionMode = iggy3d::ProductInteractionMode::Player;
}

bool activeRuleUsesCreativeDocumentIdentity() {
  iggy3d::ProductAppWindowState window;
  const bool playerActive = iggy3d::productCreativeUiActiveForWindow(window);
  window.inputDevice.interactionMode = iggy3d::ProductInteractionMode::Creative;
  const bool creativeModeActive =
      iggy3d::productCreativeUiActiveForWindow(window);
  markCreativeDocumentWindow(window);
  const bool documentCreativeActive =
      iggy3d::productCreativeUiActiveForWindow(window);

  return expect(!playerActive, "player inactive") &&
         expect(creativeModeActive, "creative mode active") &&
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
         expect(!window.creativeAuthoring.creativeUiProjection.requested,
                "inactive projection not requested") &&
         expect(!window.creativeAuthoring.creativeUiProjection.ready,
                "inactive projection not ready") &&
         expect(window.creativeAuthoring.creativeUiProjection.status ==
                    "product_creative_ui_frame_inactive",
                "inactive window status") &&
         expect(window.creativeAuthoring.creativeUiProjection.virtualWidth == 1440U,
                "inactive width copied") &&
         expect(window.creativeAuthoring.creativeUiProjection.virtualHeight == 900U,
                "inactive height copied") &&
         expect(window.creativeAuthoring.creativeUiProjection.theme == "journal",
                "inactive theme copied") &&
         expect(window.creativeAuthoring.creativeUiProjection.panelCount == 0U,
                "inactive panel count cleared") &&
         expect(window.creativeAuthoring.creativeUiProjection.primitiveCount == 0U,
                "inactive primitive count cleared") &&
         expect(window.creativeAuthoring.creativeUiProjection.textCount == 0U,
                "inactive text count cleared") &&
         expect(window.creativeAuthoring.creativeUiProjection.rectCount == 0U,
                "inactive rect count cleared") &&
         expect(window.creativeAuthoring.creativeUiProjection.rowCount == 0U,
                "inactive row count cleared") &&
         expect(window.creativeAuthoring.creativeUiProjection.hitRegionCount == 0U,
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
         expect(window.creativeAuthoring.creativeUiProjection.requested,
                "missing facade projection requested") &&
         expect(!window.creativeAuthoring.creativeUiProjection.ready,
                "missing facade projection not ready") &&
         expect(!window.creativeAuthoring.creativeUiProjection.usedFacade,
                "missing facade used facade false") &&
         expect(window.creativeAuthoring.creativeUiProjection.status ==
                    "product_creative_ui_frame_facade_missing",
                "missing facade window status") &&
         expect(window.creativeAuthoring.creativeUiProjection.primitiveCount == 0U,
                "missing facade primitive count zero") &&
         expect(window.creativeAuthoring.creativeUiProjection.hitRegionCount == 0U,
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
  cr::CreativeAppState app;
  markCreativeAppIdentity(app);
  [[maybe_unused]] cr::Facade& facade = app.facade;
  populateSelectedFacade(facade);

  iggy3d::ProductCreativeUiFrameRequest request;
  request.window = &window;
  request.creative = &app;
  const iggy3d::ProductCreativeUiFrame frame =
      iggy3d::buildProductCreativeUiFrame(request);
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expect(frame.receipt.requested, "active requested") &&
         expect(frame.receipt.active, "active frame active") &&
         expect(frame.receipt.projected, "active projected") &&
         expect(frame.receipt.recorded, "active recorded") &&
         expect(frame.receipt.ready, "active ready") &&
         expect(window.creativeAuthoring.creativeUiProjection.ready, "window ready") &&
         expect(window.creativeAuthoring.creativeUiProjection.usedFacade, "window used facade") &&
         expect(!window.creativeAuthoring.creativeUiProjection.usedModel, "window model unused") &&
         expect(window.creativeAuthoring.creativeUiProjection.panelCount > 0U,
                "panel count nonzero") &&
         expect(window.creativeAuthoring.creativeUiProjection.modelRowCount > 0U,
                "model row count nonzero") &&
         expect(window.creativeAuthoring.creativeUiProjection.primitiveCount > 0U,
                "primitive count nonzero") &&
         expect(window.creativeAuthoring.creativeUiProjection.textCount > 0U,
                "text count nonzero") &&
         expect(window.creativeAuthoring.creativeUiProjection.rectCount > 0U,
                "rect count nonzero") &&
         expect(window.creativeAuthoring.creativeUiProjection.rowCount > 0U,
                "row count nonzero") &&
         expect(window.creativeAuthoring.creativeUiProjection.hitRegionCount > 0U,
                "hit count nonzero") &&
         expect(window.creativeAuthoring.creativeUiProjection.hitRegionCount ==
                    window.creativeAuthoring.creativeUiProjection.rowCount,
                "hit count mirrors row count") &&
         expect(frame.receipt.primitiveCount ==
                    window.creativeAuthoring.creativeUiProjection.primitiveCount,
                "frame primitive count copied") &&
         expect(frame.receipt.textCount == window.creativeAuthoring.creativeUiProjection.textCount,
                "frame text count copied") &&
         expect(frame.receipt.rectCount == window.creativeAuthoring.creativeUiProjection.rectCount,
                "frame rect count copied") &&
         expect(frame.receipt.rowCount == window.creativeAuthoring.creativeUiProjection.rowCount,
                "frame row count copied") &&
         expect(frame.receipt.hitRegionCount ==
                    window.creativeAuthoring.creativeUiProjection.hitRegionCount,
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
  cr::CreativeAppState app;
  markCreativeAppIdentity(app);
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();

  iggy3d::ProductCreativeUiFrameRequest request;
  request.window = &window;
  request.creative = &app;
  const iggy3d::ProductCreativeUiFrame activeFrame =
      iggy3d::buildProductCreativeUiFrame(request);

  window.inputDevice.interactionMode = iggy3d::ProductInteractionMode::Player;
  clearCreativeDocumentIdentity(window);
  const iggy3d::ProductCreativeUiFrame inactiveFrame =
      iggy3d::buildProductCreativeUiFrame(request);

  return expect(activeFrame.receipt.ready, "first active ready") &&
         expect(window.creativeAuthoring.creativeUiProjection.status ==
                    "product_creative_ui_frame_inactive",
                "cleared inactive status") &&
         expect(!window.creativeAuthoring.creativeUiProjection.requested,
                "cleared inactive requested") &&
         expect(!window.creativeAuthoring.creativeUiProjection.ready, "cleared inactive ready") &&
         expect(!window.creativeAuthoring.creativeUiProjection.usedFacade,
                "cleared inactive facade") &&
         expect(window.creativeAuthoring.creativeUiProjection.primitiveCount == 0U,
                "cleared primitive count") &&
         expect(window.creativeAuthoring.creativeUiProjection.textCount == 0U,
                "cleared text count") &&
         expect(window.creativeAuthoring.creativeUiProjection.rectCount == 0U,
                "cleared rect count") &&
         expect(window.creativeAuthoring.creativeUiProjection.rowCount == 0U,
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
  window.frontendShell.productVulkanMenu.uiReady = true;
  window.frontendShell.productVulkanMenu.uiPartial = true;
  window.frontendShell.productVulkanMenu.uiStatus = "preexisting_ui_status";
  window.frontendShell.productVulkanMenu.uiReasonCode = "preexisting_ui_reason";
  window.frontendShell.productVulkanMenu.uiPrimitiveCount = 101;
  window.frontendShell.productVulkanMenu.uiTextCount = 102;
  window.frontendShell.productVulkanMenu.uiRectCount = 103;
  window.frontendShell.productVulkanMenu.uiRowCount = 104;
  window.frontendShell.productVulkanMenu.uiSelectedAction = "preexisting_action";

  cr::CreativeAppState app;
  markCreativeAppIdentity(app);
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  iggy3d::ProductCreativeUiFrameRequest request;
  request.window = &window;
  request.creative = &app;
  static_cast<void>(iggy3d::buildProductCreativeUiFrame(request));

  return expect(window.frontendShell.productVulkanMenu.uiReady,
                "vulkan ui ready unchanged") &&
         expect(window.frontendShell.productVulkanMenu.uiPartial,
                "vulkan ui partial unchanged") &&
         expect(window.frontendShell.productVulkanMenu.uiStatus == "preexisting_ui_status",
                "vulkan ui status unchanged") &&
         expect(window.frontendShell.productVulkanMenu.uiReasonCode ==
                    "preexisting_ui_reason",
                "vulkan ui reason unchanged") &&
         expect(window.frontendShell.productVulkanMenu.uiPrimitiveCount == 101U,
                "vulkan ui primitive count unchanged") &&
         expect(window.frontendShell.productVulkanMenu.uiTextCount == 102U,
                "vulkan ui text count unchanged") &&
         expect(window.frontendShell.productVulkanMenu.uiRectCount == 103U,
                "vulkan ui rect count unchanged") &&
         expect(window.frontendShell.productVulkanMenu.uiRowCount == 104U,
                "vulkan ui row count unchanged") &&
         expect(window.frontendShell.productVulkanMenu.uiSelectedAction ==
                    "preexisting_action",
                "vulkan ui action unchanged");
}

bool facadeStateIsNotMutatedByProjection() {
  iggy3d::ProductAppWindowState window;
  markCreativeDocumentWindow(window);
  cr::CreativeAppState app;
  markCreativeAppIdentity(app);
  [[maybe_unused]] cr::Facade& facade = app.facade;
  populateSelectedFacade(facade);

  const cr::Tool activeToolBefore = facade.toolState().activeTool;
  const cr::TargetRef selectedBefore =
      facade.selectionState().selectedTarget;

  iggy3d::ProductCreativeUiFrameRequest request;
  request.window = &window;
  request.creative = &app;
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
