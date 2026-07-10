#pragma once

#include <cstdint>
#include <string>

#include "app/frontend/SettingsMenu.hpp"

namespace iggy3d {

// Owned Vulkan-menu render state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: window/vulkan menu. Behavior-identical.
// Owned startup timing/measurement state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). App-lifecycle receipts. Behavior-identical.
struct ProductStartupState {
  std::string packagePath = "none";
  bool packageLookupMeasured = false;
  std::uint64_t packageLookupMicroseconds = 0;
  std::string packageLookupStatus = "startup_package_lookup_not_requested";
  bool packageLoadMeasured = false;
  std::uint64_t packageLoadMicroseconds = 0;
  std::string packageLoadStatus = "startup_package_load_not_requested";
  bool runtimeSessionCreateMeasured = false;
  std::uint64_t runtimeSessionCreateMicroseconds = 0;
  std::string runtimeSessionCreateStatus = "startup_runtime_session_create_not_requested";
  bool creativeWorldIdScanMeasured = false;
  std::uint64_t creativeWorldIdScanMicroseconds = 0;
  std::uint64_t creativeWorldIdScanEntryCount = 0;
  std::string creativeWorldIdScanStatus = "product_world_id_scan_not_requested";
  bool creativeDocumentIdScanMeasured = false;
  std::uint64_t creativeDocumentIdScanMicroseconds = 0;
  std::uint64_t creativeDocumentIdScanEntryCount = 0;
  std::string creativeDocumentIdScanStatus = "creative_document_id_scan_not_requested";
  bool creativeUiFirstFrameMeasured = false;
  std::uint64_t creativeUiFirstFrameMicroseconds = 0;
  std::string creativeUiFirstFrameStatus = "startup_creative_ui_first_frame_not_requested";
  bool creativeWireframeFirstFrameMeasured = false;
  std::uint64_t creativeWireframeFirstFrameMicroseconds = 0;
  std::string creativeWireframeFirstFrameStatus =
      "startup_creative_wireframe_first_frame_not_requested";
  bool vulkanRendererInitMeasured = false;
  std::uint64_t vulkanRendererInitMicroseconds = 0;
  std::string vulkanRendererInitStatus = "startup_vulkan_renderer_init_not_requested";
  bool vulkanFirstSubmitMeasured = false;
  std::uint64_t vulkanFirstSubmitMicroseconds = 0;
  std::string vulkanFirstSubmitStatus = "startup_vulkan_first_submit_not_requested";
};

struct ProductVulkanMenuState {
  bool requested = false;
  bool visible = false;
  std::string status = "vulkan_menu_not_requested";
  std::string reasonCode = "vulkan_menu_not_requested";
  std::string surface = "none";
  bool uiReady = false;
  bool uiPartial = false;
  std::string uiStatus = "product_vulkan_menu_ui_not_requested";
  std::string uiReasonCode = "product_vulkan_menu_ui_not_requested";
  std::uint64_t uiPrimitiveCount = 0;
  std::uint64_t uiTextCount = 0;
  std::uint64_t uiRectCount = 0;
  std::uint64_t uiRowCount = 0;
  std::string uiSelectedAction = "none";
};

struct FrontendWindowShell {
  bool openingMenuVisible = false;
  bool menuTextDrawn = false;
  bool selectedRowDrawn = false;
  bool mouseMenuSelectUsed = false;
  bool gamepadMenuSelectUsed = false;
  FrontendSettingsTab selectedSettingsTab = FrontendSettingsTab::None;
  std::string launchAction = "none";
  std::string launchStatus = "not_requested";
  std::string packageLoadStatus = "not_requested";
  ProductStartupState startup;
  ProductVulkanMenuState productVulkanMenu;
  std::uint64_t framesPresented = 0;
  std::uint64_t eventPollCount = 0;
  std::uint64_t menuRowCount = 0;
  std::string status = "window_not_requested";
};

}  // namespace iggy3d
