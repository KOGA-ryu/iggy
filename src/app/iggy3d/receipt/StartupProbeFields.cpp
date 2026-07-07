#include "app/iggy3d/receipt/ReceiptFields.hpp"

#include "app/iggy3d/menu/FrontendRouter.hpp"

namespace iggy3d {

void appendProductStartupProbeFields(RenderReceipt& receipt, const FrontendState& frontend, const ProductAppWindowState& window, const ProductSaveBridgeResult& saves) {
  appendReceiptField(receipt, "dev_tools_category",
                     frontendDevToolsCategoryName(frontend.devToolsCategory));
  appendReceiptField(receipt, "launch_action", window.launchAction);
  appendReceiptField(receipt, "launch_status", window.launchStatus);
  appendReceiptField(receipt, "package_load_status", window.packageLoadStatus);
  appendReceiptField(receipt, "startup_package_path",
                     window.startup.packagePath);
  appendReceiptField(receipt, "startup_package_lookup_measured",
                     window.startup.packageLookupMeasured);
  appendReceiptField(receipt, "startup_package_lookup_us",
                     window.startup.packageLookupMicroseconds);
  appendReceiptField(receipt, "startup_package_lookup_status",
                     window.startup.packageLookupStatus);
  appendReceiptField(receipt, "startup_package_load_measured",
                     window.startup.packageLoadMeasured);
  appendReceiptField(receipt, "startup_package_load_us",
                     window.startup.packageLoadMicroseconds);
  appendReceiptField(receipt, "startup_package_load_status",
                     window.startup.packageLoadStatus);
  appendReceiptField(receipt, "startup_runtime_session_create_measured",
                     window.startup.runtimeSessionCreateMeasured);
  appendReceiptField(receipt, "startup_runtime_session_create_us",
                     window.startup.runtimeSessionCreateMicroseconds);
  appendReceiptField(receipt, "startup_runtime_session_create_status",
                     window.startup.runtimeSessionCreateStatus);
  appendReceiptField(receipt, "startup_save_catalog_scan_measured",
                     saves.scanMeasured);
  appendReceiptField(receipt, "startup_save_catalog_scan_us",
                     saves.scanMicroseconds);
  appendReceiptField(receipt, "startup_save_catalog_scan_entry_count",
                     saves.scanEntryCount);
  appendReceiptField(receipt, "startup_save_catalog_scan_status",
                     saves.scanStatus);
  appendReceiptField(receipt, "startup_creative_world_id_scan_measured",
                     window.startup.creativeWorldIdScanMeasured);
  appendReceiptField(receipt, "startup_creative_world_id_scan_us",
                     window.startup.creativeWorldIdScanMicroseconds);
  appendReceiptField(receipt, "startup_creative_world_id_scan_entry_count",
                     window.startup.creativeWorldIdScanEntryCount);
  appendReceiptField(receipt, "startup_creative_world_id_scan_status",
                     window.startup.creativeWorldIdScanStatus);
  appendReceiptField(receipt, "startup_creative_document_id_scan_measured",
                     window.startup.creativeDocumentIdScanMeasured);
  appendReceiptField(receipt, "startup_creative_document_id_scan_us",
                     window.startup.creativeDocumentIdScanMicroseconds);
  appendReceiptField(receipt, "startup_creative_document_id_scan_entry_count",
                     window.startup.creativeDocumentIdScanEntryCount);
  appendReceiptField(receipt, "startup_creative_document_id_scan_status",
                     window.startup.creativeDocumentIdScanStatus);
  appendReceiptField(receipt, "startup_creative_ui_first_frame_measured",
                     window.startup.creativeUiFirstFrameMeasured);
  appendReceiptField(receipt, "startup_creative_ui_first_frame_us",
                     window.startup.creativeUiFirstFrameMicroseconds);
  appendReceiptField(receipt, "startup_creative_ui_first_frame_status",
                     window.startup.creativeUiFirstFrameStatus);
  appendReceiptField(receipt, "startup_creative_wireframe_first_frame_measured",
                     window.startup.creativeWireframeFirstFrameMeasured);
  appendReceiptField(receipt, "startup_creative_wireframe_first_frame_us",
                     window.startup.creativeWireframeFirstFrameMicroseconds);
  appendReceiptField(receipt, "startup_creative_wireframe_first_frame_status",
                     window.startup.creativeWireframeFirstFrameStatus);
  appendReceiptField(receipt, "startup_vulkan_renderer_init_measured",
                     window.startup.vulkanRendererInitMeasured);
  appendReceiptField(receipt, "startup_vulkan_renderer_init_us",
                     window.startup.vulkanRendererInitMicroseconds);
  appendReceiptField(receipt, "startup_vulkan_renderer_init_status",
                     window.startup.vulkanRendererInitStatus);
  appendReceiptField(receipt, "startup_vulkan_first_submit_measured",
                     window.startup.vulkanFirstSubmitMeasured);
  appendReceiptField(receipt, "startup_vulkan_first_submit_us",
                     window.startup.vulkanFirstSubmitMicroseconds);
  appendReceiptField(receipt, "startup_vulkan_first_submit_status",
                     window.startup.vulkanFirstSubmitStatus);
}

}  // namespace iggy3d
