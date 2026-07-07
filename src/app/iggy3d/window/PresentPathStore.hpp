#pragma once

#include "app/iggy3d/window/ProductVulkanRendererState.hpp"

#include <cstdint>
#include <string>

namespace iggy3d {

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
