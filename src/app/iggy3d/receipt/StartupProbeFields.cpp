#include "app/iggy3d/receipt/ReceiptFields.hpp"

#include "app/iggy3d/menu/FrontendRouter.hpp"

namespace iggy3d {

void appendProductStartupProbeFields(RenderReceipt& receipt, const FrontendState& frontend, const ProductAppWindowState& window, const ProductSaveBridgeResult& saves) {
  appendReceiptField(receipt, "dev_tools_category",
                     frontendDevToolsCategoryName(frontend.devToolsCategory));
  appendReceiptField(receipt, "launch_action", window.frontendShell.launchAction);
  appendReceiptField(receipt, "launch_status", window.frontendShell.launchStatus);
  appendReceiptField(receipt, "package_load_status", window.frontendShell.packageLoadStatus);
  appendReceiptField(receipt, "startup_package_path",
                     window.frontendShell.startup.packagePath);
  appendReceiptField(receipt, "startup_package_lookup_measured",
                     window.frontendShell.startup.packageLookupMeasured);
  appendReceiptField(receipt, "startup_package_lookup_us",
                     window.frontendShell.startup.packageLookupMicroseconds);
  appendReceiptField(receipt, "startup_package_lookup_status",
                     window.frontendShell.startup.packageLookupStatus);
  appendReceiptField(receipt, "startup_package_load_measured",
                     window.frontendShell.startup.packageLoadMeasured);
  appendReceiptField(receipt, "startup_package_load_us",
                     window.frontendShell.startup.packageLoadMicroseconds);
  appendReceiptField(receipt, "startup_package_load_status",
                     window.frontendShell.startup.packageLoadStatus);
  appendReceiptField(receipt, "startup_runtime_session_create_measured",
                     window.frontendShell.startup.runtimeSessionCreateMeasured);
  appendReceiptField(receipt, "startup_runtime_session_create_us",
                     window.frontendShell.startup.runtimeSessionCreateMicroseconds);
  appendReceiptField(receipt, "startup_runtime_session_create_status",
                     window.frontendShell.startup.runtimeSessionCreateStatus);
  appendReceiptField(receipt, "startup_save_catalog_scan_measured",
                     saves.scanMeasured);
  appendReceiptField(receipt, "startup_save_catalog_scan_us",
                     saves.scanMicroseconds);
  appendReceiptField(receipt, "startup_save_catalog_scan_entry_count",
                     saves.scanEntryCount);
  appendReceiptField(receipt, "startup_save_catalog_scan_status",
                     saves.scanStatus);
  appendReceiptField(receipt, "startup_creative_world_id_scan_measured",
                     window.frontendShell.startup.creativeWorldIdScanMeasured);
  appendReceiptField(receipt, "startup_creative_world_id_scan_us",
                     window.frontendShell.startup.creativeWorldIdScanMicroseconds);
  appendReceiptField(receipt, "startup_creative_world_id_scan_entry_count",
                     window.frontendShell.startup.creativeWorldIdScanEntryCount);
  appendReceiptField(receipt, "startup_creative_world_id_scan_status",
                     window.frontendShell.startup.creativeWorldIdScanStatus);
  appendReceiptField(receipt, "startup_creative_document_id_scan_measured",
                     window.frontendShell.startup.creativeDocumentIdScanMeasured);
  appendReceiptField(receipt, "startup_creative_document_id_scan_us",
                     window.frontendShell.startup.creativeDocumentIdScanMicroseconds);
  appendReceiptField(receipt, "startup_creative_document_id_scan_entry_count",
                     window.frontendShell.startup.creativeDocumentIdScanEntryCount);
  appendReceiptField(receipt, "startup_creative_document_id_scan_status",
                     window.frontendShell.startup.creativeDocumentIdScanStatus);
  appendReceiptField(receipt, "startup_creative_ui_first_frame_measured",
                     window.frontendShell.startup.creativeUiFirstFrameMeasured);
  appendReceiptField(receipt, "startup_creative_ui_first_frame_us",
                     window.frontendShell.startup.creativeUiFirstFrameMicroseconds);
  appendReceiptField(receipt, "startup_creative_ui_first_frame_status",
                     window.frontendShell.startup.creativeUiFirstFrameStatus);
  appendReceiptField(receipt, "startup_creative_wireframe_first_frame_measured",
                     window.frontendShell.startup.creativeWireframeFirstFrameMeasured);
  appendReceiptField(receipt, "startup_creative_wireframe_first_frame_us",
                     window.frontendShell.startup.creativeWireframeFirstFrameMicroseconds);
  appendReceiptField(receipt, "startup_creative_wireframe_first_frame_status",
                     window.frontendShell.startup.creativeWireframeFirstFrameStatus);
  appendReceiptField(receipt, "startup_vulkan_renderer_init_measured",
                     window.frontendShell.startup.vulkanRendererInitMeasured);
  appendReceiptField(receipt, "startup_vulkan_renderer_init_us",
                     window.frontendShell.startup.vulkanRendererInitMicroseconds);
  appendReceiptField(receipt, "startup_vulkan_renderer_init_status",
                     window.frontendShell.startup.vulkanRendererInitStatus);
  appendReceiptField(receipt, "startup_vulkan_first_submit_measured",
                     window.frontendShell.startup.vulkanFirstSubmitMeasured);
  appendReceiptField(receipt, "startup_vulkan_first_submit_us",
                     window.frontendShell.startup.vulkanFirstSubmitMicroseconds);
  appendReceiptField(receipt, "startup_vulkan_first_submit_status",
                     window.frontendShell.startup.vulkanFirstSubmitStatus);
}

}  // namespace iggy3d
