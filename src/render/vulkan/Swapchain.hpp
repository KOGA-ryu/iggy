#pragma once

#include <cstdint>
#include <vector>

#include "render/RendererConfig.hpp"
#include "render/vulkan/VulkanFunctions.hpp"

namespace iggy3d::vulkan {

enum class SwapchainState : std::uint8_t {
  Uninitialized,
  Ready,
  DirtyResize,
  DirtyOutOfDate,
  DirtySuboptimal,
  NotDrawable,
  Recreating,
  SurfaceLost,
  DeviceLost,
  Failed,
};

struct SwapchainCreateInfo {
  VkDevice device{};
  VkPhysicalDevice physicalDevice{};
  VkSurfaceKHR surface{};
  VulkanQueueFamilySelection queues;
  VulkanFunctionTables functions;
  std::uint32_t drawableWidth = 0;
  std::uint32_t drawableHeight = 0;
  RendererConfig config;
};

struct SwapchainInfo {
  VkFormat colorFormat{};
  VkColorSpaceKHR colorSpace{};
  VkPresentModeKHR presentMode{};
  VkExtent2D extent{};
  std::uint32_t imageCount = 0;
  std::uint32_t generation = 0;
  std::uint32_t recreateCount = 0;
  bool transferSourceSupported = false;
  SwapchainState state = SwapchainState::Uninitialized;
};

struct SwapchainOperationResult {
  RenderOutcome outcome = RenderOutcome::RendererNotReady;
  RenderReason reason{"swapchain_not_ready", "swapchain not ready"};
  RenderReceipt receipt;
};

struct SwapchainAcquireResult {
  RenderOutcome outcome = RenderOutcome::RendererNotReady;
  RenderReason reason{"swapchain_not_ready", "swapchain not ready"};
  std::uint32_t imageIndex = 0;
  bool imageValid = false;
  bool submitAllowed = false;
  bool presentAllowed = false;
  bool recreateRequested = false;
  RenderReceipt receipt;
};

struct SwapchainPresentResult {
  RenderOutcome outcome = RenderOutcome::RendererNotReady;
  RenderReason reason{"swapchain_not_ready", "swapchain not ready"};
  bool presented = false;
  bool recreateRequested = false;
  RenderReceipt receipt;
};

class Swapchain {
public:
  Swapchain() = default;
  ~Swapchain();

  Swapchain(const Swapchain&) = delete;
  Swapchain& operator=(const Swapchain&) = delete;

  SwapchainOperationResult create(const SwapchainCreateInfo& createInfo);
  SwapchainOperationResult recreate(std::uint32_t drawableWidth, std::uint32_t drawableHeight);
  SwapchainOperationResult markDrawableExtent(std::uint32_t width, std::uint32_t height);
  void destroy();

  SwapchainAcquireResult acquire(VkSemaphore imageAvailableSemaphore);
  SwapchainPresentResult notePresentResult(VkResult result);

  const SwapchainInfo& info() const;
  VkSwapchainKHR handle() const;
  VkImage imageAt(std::uint32_t index) const;
  VkImageView imageViewAt(std::uint32_t index) const;
  bool ready() const;

private:
  SwapchainOperationResult createInternal(std::uint32_t drawableWidth,
                                          std::uint32_t drawableHeight,
                                          VkSwapchainKHR oldSwapchain,
                                          bool recreating);
  RenderReceipt makeReceipt(std::string_view result, std::string_view reasonCode) const;
  void destroyImageViews();

  SwapchainCreateInfo createInfo_;
  SwapchainInfo info_;
  VkSwapchainKHR swapchain_{};
  std::vector<VkImage> images_;
  std::vector<VkImageView> imageViews_;
  bool hasCreateInfo_ = false;
};

std::string_view swapchainStateName(SwapchainState state);
std::string_view swapchainPresentModeName(VkPresentModeKHR mode);
std::string_view swapchainFormatName(VkFormat format);
std::string_view swapchainColorSpaceName(VkColorSpaceKHR colorSpace);

}  // namespace iggy3d::vulkan
