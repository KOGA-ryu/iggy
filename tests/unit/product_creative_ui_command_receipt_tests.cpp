#include "app/iggy3d/ReceiptBuilder.hpp"

#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/window/CreativeUiCommandFrame.hpp"

#include <cstdint>
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

iggy3d::ProductCreativeUiInputFrameReceipt commandInput(
    std::string_view semanticId,
    bool consumed = true,
    bool enabled = true) {
  iggy3d::ProductCreativeUiInputFrameReceipt receipt;
  receipt.consumed = consumed;
  receipt.enabled = enabled;
  receipt.semanticId = std::string(semanticId);
  return receipt;
}

iggy3d::ProductCreativeUiCommandFrameReceipt nullFacadeReceipt() {
  iggy3d::ProductCreativeUiCommandFrameRequest request;
  request.inputReceipt = commandInput("creative.row.tools.active_tool");
  return iggy3d::routeProductCreativeUiCommandFrame(request);
}

iggy3d::ProductCreativeUiCommandFrameReceipt appliedCommandReceipt() {
  cr::Facade facade;
  facade.reset();
  iggy3d::ProductCreativeUiCommandFrameRequest request;
  request.facade = &facade;
  request.inputReceipt = commandInput("creative.row.tools.active_tool");
  return iggy3d::routeProductCreativeUiCommandFrame(request);
}

cr::CreativeToolInputPacket pointerPress(cr::Id targetId) {
  cr::CreativeToolInputPacket input;
  input.kind = cr::CreativeToolInputKind::PointerPress;
  input.pointer.button = cr::CreativeToolPointerButton::Primary;
  input.pointer.target.value = targetId;
  return input;
}

iggy3d::ProductCreativeUiCommandFrameReceipt appliedToggleReceipt() {
  cr::Facade facade;
  facade.reset();
  const cr::CreativeObjectId roomId = facade.createRoom("Room");
  static_cast<void>(facade.dispatchToolInput(
      pointerPress(static_cast<cr::Id>(roomId))));

  iggy3d::ProductCreativeUiCommandFrameRequest request;
  request.facade = &facade;
  request.inputReceipt = commandInput("creative.row.selection.selected_target");
  return iggy3d::routeProductCreativeUiCommandFrame(request);
}

bool defaultWindowReceiptCarriesNotRequestedFields() {
  const iggy3d::ProductAppWindowState window;
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expectReceiptField(receipt,
                            "creative_ui_command_requested",
                            "false",
                            "default requested") &&
         expectReceiptField(receipt,
                            "creative_ui_command_facade_available",
                            "false",
                            "default facade") &&
         expectReceiptField(receipt,
                            "creative_ui_command_input_consumed",
                            "false",
                            "default consumed") &&
         expectReceiptField(receipt,
                            "creative_ui_command_input_enabled",
                            "false",
                            "default enabled") &&
         expectReceiptField(receipt,
                            "creative_ui_command_accepted",
                            "false",
                            "default accepted") &&
         expectReceiptField(receipt,
                            "creative_ui_command_changed",
                            "false",
                            "default changed") &&
         expectReceiptField(receipt,
                            "creative_ui_command_kind",
                            "none",
                            "default kind") &&
         expectReceiptField(receipt,
                            "creative_ui_command_tool_before",
                            "Select",
                            "default tool before") &&
         expectReceiptField(receipt,
                            "creative_ui_command_tool_after",
                            "Select",
                            "default tool after") &&
         expectReceiptField(receipt,
                            "creative_ui_command_semantic_id",
                            "none",
                            "default semantic") &&
         expectReceiptField(receipt,
                            "creative_ui_command_status",
                            "product_creative_ui_command_not_requested",
                            "default status") &&
         expectReceiptField(receipt,
                            "creative_ui_command_reason_code",
                            "product_creative_ui_command_not_requested",
                            "default reason") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_requested",
                            "false",
                            "default mutation requested") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_accepted",
                            "false",
                            "default mutation accepted") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_changed",
                            "false",
                            "default mutation changed") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_status",
                            "Unknown",
                            "default mutation status") &&
         expectReceiptField(receipt,
                            "creative_ui_command_document_mutation_status",
                            "Unknown",
                            "default document mutation status") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_kind",
                            "Unknown",
                            "default mutation kind") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_target",
                            "0",
                            "default mutation target") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_object_id",
                            "0",
                            "default mutation object") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_object_kind",
                            "Unknown",
                            "default mutation object kind") &&
         expectReceiptField(receipt,
                            "creative_ui_command_visible_before",
                            "false",
                            "default visible before") &&
         expectReceiptField(receipt,
                            "creative_ui_command_visible_after",
                            "false",
                            "default visible after") &&
         expectReceiptField(receipt,
                            "creative_ui_command_revision_before",
                            "0",
                            "default revision before") &&
         expectReceiptField(receipt,
                            "creative_ui_command_revision_after",
                            "0",
                            "default revision after") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_message",
                            "none",
                            "default mutation message");
}

bool defaultCommandReceiptRecordsSafely() {
  iggy3d::ProductAppWindowState window;
  const iggy3d::ProductCreativeUiCommandFrameReceipt commandReceipt;
  iggy3d::recordProductCreativeUiCommandFrame(window, commandReceipt);
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expectReceiptField(receipt,
                            "creative_ui_command_requested",
                            "false",
                            "record default requested") &&
         expectReceiptField(receipt,
                            "creative_ui_command_kind",
                            "none",
                            "record default kind") &&
         expectReceiptField(receipt,
                            "creative_ui_command_tool_before",
                            "Select",
                            "record default before") &&
         expectReceiptField(receipt,
                            "creative_ui_command_tool_after",
                            "Select",
                            "record default after") &&
         expectReceiptField(receipt,
                            "creative_ui_command_semantic_id",
                            "none",
                            "record default semantic") &&
         expectReceiptField(receipt,
                            "creative_ui_command_status",
                            "product_creative_ui_command_not_requested",
                            "record default status") &&
         expectReceiptField(receipt,
                            "creative_ui_command_reason_code",
                            "product_creative_ui_command_not_requested",
                            "record default reason") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_status",
                            "Unknown",
                            "record default mutation status") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_message",
                            "none",
                            "record default mutation message");
}

bool nullFacadeCommandReceiptRecordsFields() {
  const iggy3d::ProductCreativeUiCommandFrameReceipt commandReceipt =
      nullFacadeReceipt();
  iggy3d::ProductAppWindowState window;
  iggy3d::recordProductCreativeUiCommandFrame(window, commandReceipt);
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expectReceiptField(receipt,
                            "creative_ui_command_requested",
                            "true",
                            "null facade requested") &&
         expectReceiptField(receipt,
                            "creative_ui_command_facade_available",
                            "false",
                            "null facade unavailable") &&
         expectReceiptField(receipt,
                            "creative_ui_command_input_consumed",
                            "true",
                            "null facade consumed") &&
         expectReceiptField(receipt,
                            "creative_ui_command_input_enabled",
                            "true",
                            "null facade enabled") &&
         expectReceiptField(receipt,
                            "creative_ui_command_kind",
                            "none",
                            "null facade kind") &&
         expectReceiptField(receipt,
                            "creative_ui_command_tool_before",
                            "Select",
                            "null facade before") &&
         expectReceiptField(receipt,
                            "creative_ui_command_tool_after",
                            "Select",
                            "null facade after") &&
         expectReceiptField(receipt,
                            "creative_ui_command_semantic_id",
                            "creative.row.tools.active_tool",
                            "null facade semantic") &&
         expectReceiptField(receipt,
                            "creative_ui_command_status",
                            "product_creative_ui_command_facade_missing",
                            "null facade status") &&
         expectReceiptField(receipt,
                            "creative_ui_command_reason_code",
                            "product_creative_ui_command_facade_missing",
                            "null facade reason");
}

bool appliedCommandReceiptRecordsFields() {
  const iggy3d::ProductCreativeUiCommandFrameReceipt commandReceipt =
      appliedCommandReceipt();
  iggy3d::ProductAppWindowState window;
  iggy3d::recordProductCreativeUiCommandFrame(window, commandReceipt);
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expectReceiptField(receipt,
                            "creative_ui_command_requested",
                            "true",
                            "applied requested") &&
         expectReceiptField(receipt,
                            "creative_ui_command_facade_available",
                            "true",
                            "applied facade") &&
         expectReceiptField(receipt,
                            "creative_ui_command_input_consumed",
                            "true",
                            "applied consumed") &&
         expectReceiptField(receipt,
                            "creative_ui_command_input_enabled",
                            "true",
                            "applied enabled") &&
         expectReceiptField(receipt,
                            "creative_ui_command_accepted",
                            "true",
                            "applied accepted") &&
         expectReceiptField(receipt,
                            "creative_ui_command_changed",
                            "true",
                            "applied changed") &&
         expectReceiptField(receipt,
                            "creative_ui_command_kind",
                            "cycle_next_tool",
                            "applied kind") &&
         expectReceiptField(receipt,
                            "creative_ui_command_tool_before",
                            "Select",
                            "applied before") &&
         expectReceiptField(receipt,
                            "creative_ui_command_tool_after",
                            "Inspect",
                            "applied after") &&
         expectReceiptField(receipt,
                            "creative_ui_command_semantic_id",
                            "creative.row.tools.active_tool",
                            "applied semantic") &&
         expectReceiptField(receipt,
                            "creative_ui_command_status",
                            "product_creative_ui_command_applied",
                            "applied status") &&
         expectReceiptField(receipt,
                            "creative_ui_command_reason_code",
                            "product_creative_ui_command_applied",
                            "applied reason");
}

bool toggleCommandReceiptRecordsMutationFields() {
  const iggy3d::ProductCreativeUiCommandFrameReceipt commandReceipt =
      appliedToggleReceipt();
  iggy3d::ProductAppWindowState window;
  iggy3d::recordProductCreativeUiCommandFrame(window, commandReceipt);
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  const std::string objectId = std::to_string(commandReceipt.mutationObjectId);
  const std::string revisionBefore =
      std::to_string(commandReceipt.revisionBefore);
  const std::string revisionAfter =
      std::to_string(commandReceipt.revisionAfter);

  return expectReceiptField(receipt,
                            "creative_ui_command_kind",
                            "toggle_selected_object_visibility",
                            "toggle kind") &&
         expectReceiptField(receipt,
                            "creative_ui_command_status",
                            "product_creative_ui_command_applied",
                            "toggle status") &&
         expectReceiptField(receipt,
                            "creative_ui_command_accepted",
                            "true",
                            "toggle accepted") &&
         expectReceiptField(receipt,
                            "creative_ui_command_changed",
                            "true",
                            "toggle changed") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_requested",
                            "true",
                            "toggle mutation requested") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_accepted",
                            "true",
                            "toggle mutation accepted") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_changed",
                            "true",
                            "toggle mutation changed") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_status",
                            "Applied",
                            "toggle mutation status") &&
         expectReceiptField(receipt,
                            "creative_ui_command_document_mutation_status",
                            "Applied",
                            "toggle document mutation status") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_kind",
                            "SetVisible",
                            "toggle mutation kind") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_target",
                            objectId,
                            "toggle mutation target") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_object_id",
                            objectId,
                            "toggle mutation object") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_object_kind",
                            "Room",
                            "toggle mutation object kind") &&
         expectReceiptField(receipt,
                            "creative_ui_command_visible_before",
                            "true",
                            "toggle visible before") &&
         expectReceiptField(receipt,
                            "creative_ui_command_visible_after",
                            "false",
                            "toggle visible after") &&
         expectReceiptField(receipt,
                            "creative_ui_command_revision_before",
                            revisionBefore,
                            "toggle revision before") &&
         expectReceiptField(receipt,
                            "creative_ui_command_revision_after",
                            revisionAfter,
                            "toggle revision after") &&
         expectReceiptField(receipt,
                            "creative_ui_command_mutation_message",
                            "document mutation applied through object mutation pipeline",
                            "toggle mutation message");
}

bool recorderPreservesNeighboringFields() {
  iggy3d::ProductAppWindowState window;
  window.status = "window_before";
  window.creativeUiInputRequested = true;
  window.creativeUiInputConsumed = true;
  window.creativeUiInputStatus = "input_before";
  window.creativeUiInputDownstreamClickRequested = true;
  window.creativeUiInputDownstreamClickSuppressed = true;
  window.creativeUiInputDownstreamClickStatus = "downstream_before";
  window.creativeViewportPickRequested = true;
  window.creativeViewportPickStatus = "viewport_before";
  window.creativeUiProjectionRequested = true;
  window.creativeUiProjectionStatus = "projection_before";
  window.productVulkanMenuUiReady = true;
  window.productVulkanMenuUiStatus = "vulkan_before";
  window.productVulkanMenuUiSelectedAction = "resume";

  const iggy3d::ProductCreativeUiCommandFrameReceipt commandReceipt =
      appliedCommandReceipt();
  iggy3d::recordProductCreativeUiCommandFrame(window, commandReceipt);
  const iggy3d::RenderReceipt receipt = receiptFor(window);

  return expect(window.status == "window_before", "window status kept") &&
         expect(window.creativeUiInputRequested, "input requested kept") &&
         expect(window.creativeUiInputConsumed, "input consumed kept") &&
         expect(window.creativeUiInputStatus == "input_before",
                "input status kept") &&
         expect(window.creativeUiInputDownstreamClickRequested,
                "downstream requested kept") &&
         expect(window.creativeUiInputDownstreamClickSuppressed,
                "downstream suppressed kept") &&
         expect(window.creativeUiInputDownstreamClickStatus ==
                    "downstream_before",
                "downstream status kept") &&
         expect(window.creativeViewportPickRequested,
                "viewport requested kept") &&
         expect(window.creativeViewportPickStatus == "viewport_before",
                "viewport status kept") &&
         expect(window.creativeUiProjectionRequested,
                "projection requested kept") &&
         expect(window.creativeUiProjectionStatus == "projection_before",
                "projection status kept") &&
         expect(window.productVulkanMenuUiReady, "vulkan ready kept") &&
         expect(window.productVulkanMenuUiStatus == "vulkan_before",
                "vulkan status kept") &&
         expect(window.productVulkanMenuUiSelectedAction == "resume",
                "vulkan action kept") &&
         expectReceiptField(receipt,
                            "window_status",
                            "window_before",
                            "receipt window status kept") &&
         expectReceiptField(receipt,
                            "creative_ui_input_status",
                            "input_before",
                            "receipt input status kept") &&
         expectReceiptField(receipt,
                            "creative_ui_input_downstream_click_status",
                            "downstream_before",
                            "receipt downstream status kept") &&
         expectReceiptField(receipt,
                            "creative_viewport_pick_status",
                            "viewport_before",
                            "receipt viewport status kept") &&
         expectReceiptField(receipt,
                            "creative_ui_projection_status",
                            "projection_before",
                            "receipt projection status kept") &&
         expectReceiptField(receipt,
                            "product_vulkan_menu_ui_status",
                            "vulkan_before",
                            "receipt vulkan status kept") &&
         expectReceiptField(receipt,
                            "creative_ui_command_status",
                            "product_creative_ui_command_applied",
                            "receipt command status recorded");
}

}  // namespace

int main() {
  const bool ok = defaultWindowReceiptCarriesNotRequestedFields() &&
                  defaultCommandReceiptRecordsSafely() &&
                  nullFacadeCommandReceiptRecordsFields() &&
                  appliedCommandReceiptRecordsFields() &&
                  toggleCommandReceiptRecordsMutationFields() &&
                  recorderPreservesNeighboringFields();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
