#include "app/iggy3d/receipt/ReceiptFields.hpp"

#include <array>
#include <string_view>

#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"

namespace iggy3d {

namespace {

struct StartupProbeReceiptContext {
  const FrontendState& frontend;
  const ProductAppWindowState& window;
  const ProductSaveBridgeResult& saves;
};

struct StartupProbeReceiptFieldRow {
  std::string_view key;
  void (*append)(RenderReceipt& receipt,
                 const StartupProbeReceiptContext& context,
                 std::string_view key);
};

const std::array<StartupProbeReceiptFieldRow, 38>
    kStartupProbeReceiptFields{{
        {"dev_tools_category",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(
               receipt,
               key,
               frontendDevToolsCategoryName(
                   context.frontend.devToolsCategory));
         }},
        {"launch_action",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.launchAction);
         }},
        {"launch_status",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.launchStatus);
         }},
        {"package_load_status",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.packageLoadStatus);
         }},
        {"startup_package_path",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.startup.packagePath);
         }},
        {"startup_package_lookup_measured",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.startup.packageLookupMeasured);
         }},
        {"startup_package_lookup_us",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.startup.packageLookupMicroseconds);
         }},
        {"startup_package_lookup_status",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.startup.packageLookupStatus);
         }},
        {"startup_package_load_measured",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.startup.packageLoadMeasured);
         }},
        {"startup_package_load_us",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.startup.packageLoadMicroseconds);
         }},
        {"startup_package_load_status",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.startup.packageLoadStatus);
         }},
        {"startup_runtime_session_create_measured",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.startup.runtimeSessionCreateMeasured);
         }},
        {"startup_runtime_session_create_us",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.startup.runtimeSessionCreateMicroseconds);
         }},
        {"startup_runtime_session_create_status",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.startup.runtimeSessionCreateStatus);
         }},
        {"startup_save_catalog_scan_measured",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.saves.scanMeasured);
         }},
        {"startup_save_catalog_scan_us",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.saves.scanMicroseconds);
         }},
        {"startup_save_catalog_scan_entry_count",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.saves.scanEntryCount);
         }},
        {"startup_save_catalog_scan_status",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.saves.scanStatus);
         }},
        {"startup_creative_world_id_scan_measured",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.startup.creativeWorldIdScanMeasured);
         }},
        {"startup_creative_world_id_scan_us",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.startup.creativeWorldIdScanMicroseconds);
         }},
        {"startup_creative_world_id_scan_entry_count",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.startup.creativeWorldIdScanEntryCount);
         }},
        {"startup_creative_world_id_scan_status",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.startup.creativeWorldIdScanStatus);
         }},
        {"startup_creative_document_id_scan_measured",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.startup.creativeDocumentIdScanMeasured);
         }},
        {"startup_creative_document_id_scan_us",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.startup.creativeDocumentIdScanMicroseconds);
         }},
        {"startup_creative_document_id_scan_entry_count",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.startup.creativeDocumentIdScanEntryCount);
         }},
        {"startup_creative_document_id_scan_status",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.startup.creativeDocumentIdScanStatus);
         }},
        {"startup_creative_ui_first_frame_measured",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.startup.creativeUiFirstFrameMeasured);
         }},
        {"startup_creative_ui_first_frame_us",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.startup.creativeUiFirstFrameMicroseconds);
         }},
        {"startup_creative_ui_first_frame_status",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.startup.creativeUiFirstFrameStatus);
         }},
        {"startup_creative_wireframe_first_frame_measured",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.startup.creativeWireframeFirstFrameMeasured);
         }},
        {"startup_creative_wireframe_first_frame_us",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.startup.creativeWireframeFirstFrameMicroseconds);
         }},
        {"startup_creative_wireframe_first_frame_status",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.startup.creativeWireframeFirstFrameStatus);
         }},
        {"startup_vulkan_renderer_init_measured",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.startup.vulkanRendererInitMeasured);
         }},
        {"startup_vulkan_renderer_init_us",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.startup.vulkanRendererInitMicroseconds);
         }},
        {"startup_vulkan_renderer_init_status",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.startup.vulkanRendererInitStatus);
         }},
        {"startup_vulkan_first_submit_measured",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.startup.vulkanFirstSubmitMeasured);
         }},
        {"startup_vulkan_first_submit_us",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.startup.vulkanFirstSubmitMicroseconds);
         }},
        {"startup_vulkan_first_submit_status",
         [](RenderReceipt& receipt,
            const StartupProbeReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key, context.window.frontendShell.startup.vulkanFirstSubmitStatus);
         }},
    }};

}  // namespace

void appendProductStartupProbeFields(RenderReceipt& receipt, const FrontendState& frontend, const ProductAppWindowState& window, const ProductSaveBridgeResult& saves) {
  const StartupProbeReceiptContext context{frontend, window, saves};

  for (const StartupProbeReceiptFieldRow& row : kStartupProbeReceiptFields) {
    row.append(receipt, context, row.key);
  }
}

}  // namespace iggy3d
