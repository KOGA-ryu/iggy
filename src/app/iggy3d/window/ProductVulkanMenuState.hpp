#pragma once

#include <cstdint>
#include <string>

namespace iggy3d {

// Owned Vulkan-menu render state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: window/vulkan menu. Behavior-identical.
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

}  // namespace iggy3d
