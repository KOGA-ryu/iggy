#include "app/iggy3d/ReceiptBuilder.hpp"

#include "ProductReceiptTestSupport.hpp"

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/ui/UiProjection.hpp"
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

cr::CreativeUiModel populatedCreativeUiModel() {
  cr::CreativeUiBuildRequest request = cr::makeDefaultCreativeUiBuildRequest();
  request.selectionState.selectedTarget.value = 42;
  request.measurementState.active = true;
  request.measurementState.hasMeasurement = true;
  request.measurementState.startPoint = {1.0, 2.0, cr::TargetRef{7}};
  request.measurementState.currentPoint = {3.0, 4.0, cr::TargetRef{9}};
  request.measurementState.sampleCount = 2;
  request.ghostState.visible = true;
  request.ghostState.sourceTool = cr::Tool::Measure;
  request.ghostState.rawPoint = {1.2, 2.7};
  request.ghostState.snappedPoint = {1.0, 3.0};
  request.ghostState.target.value = 99;
  request.ghostState.snapAccepted = true;
  request.ghostState.snapApplied = true;
  request.ghostState.snapChanged = true;
  request.ghostState.updateCount = 5;
  return cr::buildCreativeUiModel(request).model;
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

bool expectRecordedCounts(
    const iggy3d::RenderReceipt& receipt,
    const iggy3d::ProductCreativeUiProjection& projection) {
  return expectReceiptCount(receipt,
                            "creative_ui_projection_panel_count",
                            projection.receipt.panelCount,
                            "panel count") &&
         expectReceiptCount(receipt,
                            "creative_ui_projection_model_row_count",
                            projection.receipt.modelRowCount,
                            "model row count") &&
         expectReceiptCount(receipt,
                            "creative_ui_projection_primitive_count",
                            projection.receipt.primitiveCount,
                            "primitive count") &&
         expectReceiptCount(receipt,
                            "creative_ui_projection_text_count",
                            projection.receipt.textCount,
                            "text count") &&
         expectReceiptCount(receipt,
                            "creative_ui_projection_rect_count",
                            projection.receipt.rectCount,
                            "rect count") &&
         expectReceiptCount(receipt,
                            "creative_ui_projection_row_count",
                            projection.receipt.rowCount,
                            "row count") &&
         expectReceiptCount(receipt,
                            "creative_ui_projection_disabled_row_count",
                            projection.receipt.disabledRowCount,
                            "disabled row count") &&
         expectReceiptCount(receipt,
                            "creative_ui_projection_hit_region_count",
                            projection.receipt.hitRegionCount,
                            "hit count");
}

bool defaultWindowReceiptCarriesNotRequestedFields() {
  const iggy3d::ProductAppWindowState window;
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expectReceiptField(receipt,
                            "creative_ui_projection_requested",
                            "false",
                            "default requested") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_ready",
                            "false",
                            "default ready") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_partial",
                            "false",
                            "default partial") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_status",
                            "creative_ui_projection_not_requested",
                            "default status") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_reason_code",
                            "creative_ui_projection_not_requested",
                            "default reason") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_used_model",
                            "false",
                            "default used model") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_used_facade",
                            "false",
                            "default used facade") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_virtual_width",
                            "0",
                            "default width") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_virtual_height",
                            "0",
                            "default height") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_theme",
                            "none",
                            "default theme") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_panel_count",
                            "0",
                            "default panel count") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_model_row_count",
                            "0",
                            "default model row count") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_primitive_count",
                            "0",
                            "default primitive count") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_text_count",
                            "0",
                            "default text count") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_rect_count",
                            "0",
                            "default rect count") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_row_count",
                            "0",
                            "default row count") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_disabled_row_count",
                            "0",
                            "default disabled row count") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_hit_region_count",
                            "0",
                            "default hit count");
}

bool modelProjectionRecordsReceiptFields() {
  const cr::CreativeUiModel model = populatedCreativeUiModel();
  iggy3d::ProductCreativeUiProjectionRequest request;
  request.model = &model;
  request.virtualWidth = 1600;
  request.virtualHeight = 900;
  request.theme = iggy3d::ProductUiThemeId::Journal;
  const iggy3d::ProductCreativeUiProjection projection =
      iggy3d::buildProductCreativeUiProjection(request);

  iggy3d::ProductAppWindowState window;
  const std::string statusBefore = window.frontendShell.status;
  iggy3d::recordProductCreativeUiProjection(window, projection.receipt);
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expect(window.frontendShell.status == statusBefore,
                "window status untouched") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_requested",
                            "true",
                            "model requested") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_ready",
                            "true",
                            "model ready") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_used_model",
                            "true",
                            "model used") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_used_facade",
                            "false",
                            "facade unused") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_status",
                            projection.receipt.status,
                            "model status") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_reason_code",
                            projection.receipt.reasonCode,
                            "model reason") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_virtual_width",
                            "1600",
                            "model width") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_virtual_height",
                            "900",
                            "model height") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_theme",
                            "journal",
                            "model theme") &&
         expect(window.creativeAuthoring.creativeUiProjection.panelCount > 0U,
                "model panel count nonzero") &&
         expect(window.creativeAuthoring.creativeUiProjection.modelRowCount > 0U,
                "model row count nonzero") &&
         expect(window.creativeAuthoring.creativeUiProjection.primitiveCount > 0U,
                "model primitive count nonzero") &&
         expect(window.creativeAuthoring.creativeUiProjection.textCount > 0U,
                "model text count nonzero") &&
         expect(window.creativeAuthoring.creativeUiProjection.rectCount > 0U,
                "model rect count nonzero") &&
         expect(window.creativeAuthoring.creativeUiProjection.rowCount > 0U,
                "model draw row count nonzero") &&
         expect(window.creativeAuthoring.creativeUiProjection.hitRegionCount > 0U,
                "model hit count nonzero") &&
         expect(window.creativeAuthoring.creativeUiProjection.hitRegionCount ==
                    window.creativeAuthoring.creativeUiProjection.rowCount,
                "model hit count mirrors row count") &&
         expectRecordedCounts(receipt, projection);
}

bool facadeProjectionRecordsReceiptFields() {
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  populateSelectedFacade(facade);

  iggy3d::ProductCreativeUiProjectionRequest request;
  request.creative = &app;
  const iggy3d::ProductCreativeUiProjection projection =
      iggy3d::buildProductCreativeUiProjection(request);

  iggy3d::ProductAppWindowState window;
  iggy3d::recordProductCreativeUiProjection(window, projection.receipt);
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expectReceiptField(receipt,
                            "creative_ui_projection_ready",
                            "true",
                            "facade ready") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_used_model",
                            "false",
                            "facade model unused") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_used_facade",
                            "true",
                            "facade used") &&
         expect(window.creativeAuthoring.creativeUiProjection.panelCount > 0U,
                "facade panel count nonzero") &&
         expect(window.creativeAuthoring.creativeUiProjection.modelRowCount > 0U,
                "facade model row count nonzero") &&
         expect(window.creativeAuthoring.creativeUiProjection.primitiveCount > 0U,
                "facade primitive count nonzero") &&
         expectRecordedCounts(receipt, projection);
}

bool failClosedProjectionRecordsMissingModel() {
  const iggy3d::ProductCreativeUiProjection projection =
      iggy3d::buildProductCreativeUiProjection({});

  iggy3d::ProductAppWindowState window;
  iggy3d::recordProductCreativeUiProjection(window, projection.receipt);
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expectReceiptField(receipt,
                            "creative_ui_projection_requested",
                            "true",
                            "missing requested") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_ready",
                            "false",
                            "missing not ready") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_status",
                            "product_creative_ui_model_missing",
                            "missing status") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_reason_code",
                            "product_creative_ui_model_missing",
                            "missing reason") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_panel_count",
                            "0",
                            "missing panel count") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_model_row_count",
                            "0",
                            "missing model row count") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_primitive_count",
                            "0",
                            "missing primitive count") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_text_count",
                            "0",
                            "missing text count") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_rect_count",
                            "0",
                            "missing rect count") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_row_count",
                            "0",
                            "missing row count") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_hit_region_count",
                            "0",
                            "missing hit count");
}

bool recordingDoesNotOverwriteProductVulkanMenuUiFields() {
  const cr::CreativeUiModel model = populatedCreativeUiModel();
  iggy3d::ProductCreativeUiProjectionRequest request;
  request.model = &model;
  const iggy3d::ProductCreativeUiProjection projection =
      iggy3d::buildProductCreativeUiProjection(request);

  iggy3d::ProductAppWindowState window;
  window.frontendShell.productVulkanMenu.uiReady = true;
  window.frontendShell.productVulkanMenu.uiPartial = true;
  window.frontendShell.productVulkanMenu.uiStatus = "preexisting_ui_status";
  window.frontendShell.productVulkanMenu.uiReasonCode = "preexisting_ui_reason";
  window.frontendShell.productVulkanMenu.uiPrimitiveCount = 101;
  window.frontendShell.productVulkanMenu.uiTextCount = 102;
  window.frontendShell.productVulkanMenu.uiRectCount = 103;
  window.frontendShell.productVulkanMenu.uiRowCount = 104;
  window.frontendShell.productVulkanMenu.uiSelectedAction = "preexisting_action";

  iggy3d::recordProductCreativeUiProjection(window, projection.receipt);
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expect(window.frontendShell.productVulkanMenu.uiReady,
                "vulkan ui ready untouched") &&
         expect(window.frontendShell.productVulkanMenu.uiPartial,
                "vulkan ui partial untouched") &&
         expect(window.frontendShell.productVulkanMenu.uiStatus == "preexisting_ui_status",
                "vulkan ui status untouched") &&
         expect(window.frontendShell.productVulkanMenu.uiReasonCode ==
                    "preexisting_ui_reason",
                "vulkan ui reason untouched") &&
         expect(window.frontendShell.productVulkanMenu.uiPrimitiveCount == 101U,
                "vulkan ui primitive count untouched") &&
         expect(window.frontendShell.productVulkanMenu.uiTextCount == 102U,
                "vulkan ui text count untouched") &&
         expect(window.frontendShell.productVulkanMenu.uiRectCount == 103U,
                "vulkan ui rect count untouched") &&
         expect(window.frontendShell.productVulkanMenu.uiRowCount == 104U,
                "vulkan ui row count untouched") &&
         expect(window.frontendShell.productVulkanMenu.uiSelectedAction ==
                    "preexisting_action",
                "vulkan ui action untouched") &&
         expectReceiptField(receipt,
                            "product_vulkan_menu_ui_status",
                            "preexisting_ui_status",
                            "receipt keeps vulkan ui status") &&
         expectReceiptField(receipt,
                            "product_vulkan_menu_ui_selected_action",
                            "preexisting_action",
                            "receipt keeps vulkan ui action");
}

}  // namespace

int main() {
  const bool ok = defaultWindowReceiptCarriesNotRequestedFields() &&
                  modelProjectionRecordsReceiptFields() &&
                  facadeProjectionRecordsReceiptFields() &&
                  failClosedProjectionRecordsMissingModel() &&
                  recordingDoesNotOverwriteProductVulkanMenuUiFields();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
