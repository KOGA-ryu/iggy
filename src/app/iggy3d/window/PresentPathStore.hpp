#pragma once

#include <cstdint>
#include <string>

namespace iggy3d {

// Owned Vulkan-renderer lifecycle-flag state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: window/vulkan. Behavior-identical.
struct ProductVulkanRendererState {
  bool requested = false;
  bool created = false;
  bool ready = false;
};

struct PresentPathStore {
  ProductVulkanRendererState productVulkanRenderer;
  bool productVulkanSurfaceCreated = false;
  bool productVulkanSwapchainReady = false;
  bool productVulkanFrameSubmitted = false;
  std::uint64_t productVulkanFrameSubmittedCount = 0;
  std::string productVulkanStatus = "not_requested";
  std::string productVulkanReasonCode = "not_requested";
  std::string productVulkanRenderingPath = "none";
  std::string productVulkanRecordMode = "none";
};

}  // namespace iggy3d
