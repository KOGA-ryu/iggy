#include "render/vulkan/Swapchain.hpp"

#include <algorithm>
#include <limits>
#include <string>

#include "render/vulkan/VulkanResult.hpp"

namespace iggy3d::vulkan {
namespace {

RenderReason reasonFor(std::string_view code) {
  if (code == "vulkan_smoke_pass") {
    return {code, "vulkan smoke pass"};
  }
  if (code == "swapchain_create_failed") {
    return {code, "swapchain create failed"};
  }
  if (code == "swapchain_image_query_failed") {
    return {code, "swapchain image query failed"};
  }
  if (code == "swapchain_image_view_create_failed") {
    return {code, "swapchain image view create failed"};
  }
  if (code == "swapchain_not_drawable") {
    return {code, "swapchain not drawable"};
  }
  if (code == "swapchain_recreate_requested") {
    return {code, "swapchain recreate requested"};
  }
  if (code == "swapchain_recreate_failed") {
    return {code, "swapchain recreate failed"};
  }
  if (code == "swapchain_acquire_failed") {
    return {code, "swapchain acquire failed"};
  }
  if (code == "swapchain_present_failed") {
    return {code, "swapchain present failed"};
  }
  if (code == "swapchain_out_of_date") {
    return {code, "swapchain out of date"};
  }
  if (code == "swapchain_suboptimal") {
    return {code, "swapchain suboptimal"};
  }
  if (code == "surface_lost") {
    return {code, "surface lost"};
  }
  if (code == "device_lost") {
    return {code, "device lost"};
  }
  return {"swapchain_create_failed", "swapchain create failed"};
}

std::string extentString(VkExtent2D extent) {
  return std::to_string(extent.width) + "x" + std::to_string(extent.height);
}

VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
  for (const VkSurfaceFormatKHR& format : formats) {
    if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return format;
    }
  }
  for (const VkSurfaceFormatKHR& format : formats) {
    if (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR &&
        (format.format == VK_FORMAT_R8G8B8A8_SRGB ||
         format.format == VK_FORMAT_A8B8G8R8_SRGB_PACK32)) {
      return format;
    }
  }
  return formats.front();
}

VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& modes,
                                   PresentModeRequest request) {
  const auto hasMode = [&modes](VkPresentModeKHR mode) {
    return std::find(modes.begin(), modes.end(), mode) != modes.end();
  };
  if (request == PresentModeRequest::Auto || request == PresentModeRequest::Fifo) {
    return VK_PRESENT_MODE_FIFO_KHR;
  }
  if (request == PresentModeRequest::Mailbox && hasMode(VK_PRESENT_MODE_MAILBOX_KHR)) {
    return VK_PRESENT_MODE_MAILBOX_KHR;
  }
  if (request == PresentModeRequest::Immediate && hasMode(VK_PRESENT_MODE_IMMEDIATE_KHR)) {
    return VK_PRESENT_MODE_IMMEDIATE_KHR;
  }
  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities,
                        std::uint32_t drawableWidth,
                        std::uint32_t drawableHeight) {
  if (capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max()) {
    return capabilities.currentExtent;
  }
  VkExtent2D extent{drawableWidth, drawableHeight};
  extent.width = std::clamp(extent.width, capabilities.minImageExtent.width,
                            capabilities.maxImageExtent.width);
  extent.height = std::clamp(extent.height, capabilities.minImageExtent.height,
                             capabilities.maxImageExtent.height);
  return extent;
}

std::uint32_t chooseImageCount(const VkSurfaceCapabilitiesKHR& capabilities) {
  std::uint32_t imageCount = capabilities.minImageCount + 1U;
  if (capabilities.maxImageCount != 0U) {
    imageCount = std::min(imageCount, capabilities.maxImageCount);
  }
  return imageCount;
}

}  // namespace

Swapchain::~Swapchain() {
  destroy();
}

std::string_view swapchainStateName(SwapchainState state) {
  switch (state) {
    case SwapchainState::Uninitialized:
      return "uninitialized";
    case SwapchainState::Ready:
      return "ready";
    case SwapchainState::DirtyResize:
      return "dirty_resize";
    case SwapchainState::DirtyOutOfDate:
      return "dirty_out_of_date";
    case SwapchainState::DirtySuboptimal:
      return "dirty_suboptimal";
    case SwapchainState::NotDrawable:
      return "not_drawable";
    case SwapchainState::Recreating:
      return "recreating";
    case SwapchainState::SurfaceLost:
      return "surface_lost";
    case SwapchainState::DeviceLost:
      return "device_lost";
    case SwapchainState::Failed:
      return "failed";
  }
  return "failed";
}

std::string_view swapchainPresentModeName(VkPresentModeKHR mode) {
  switch (mode) {
    case VK_PRESENT_MODE_FIFO_KHR:
      return "fifo";
    case VK_PRESENT_MODE_MAILBOX_KHR:
      return "mailbox";
    case VK_PRESENT_MODE_IMMEDIATE_KHR:
      return "immediate";
    default:
      return "unknown";
  }
}

std::string_view swapchainFormatName(VkFormat format) {
  switch (format) {
    case VK_FORMAT_B8G8R8A8_SRGB:
      return "b8g8r8a8_srgb";
    case VK_FORMAT_R8G8B8A8_SRGB:
      return "r8g8b8a8_srgb";
    case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
      return "a8b8g8r8_srgb_pack32";
    default:
      return "unknown";
  }
}

std::string_view swapchainColorSpaceName(VkColorSpaceKHR colorSpace) {
  switch (colorSpace) {
    case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:
      return "srgb_nonlinear";
    default:
      return "unknown";
  }
}

RenderReceipt Swapchain::makeReceipt(std::string_view result,
                                     std::string_view reasonCode) const {
  RenderReceipt receipt;
  appendReceiptField(receipt, "receipt_version", "1");
  appendReceiptField(receipt, "repo", "iggy3d");
  appendReceiptField(receipt, "file_plan", "src/render/vulkan/Swapchain.cpp");
  appendReceiptField(receipt, "packet_order", "5");
  appendReceiptField(receipt, "backend", "vulkan");
  appendReceiptField(receipt, "swapchain_state", swapchainStateName(info_.state));
  appendReceiptField(receipt, "swapchain_generation", static_cast<std::uint64_t>(info_.generation));
  appendReceiptField(receipt, "swapchain_format", swapchainFormatName(info_.colorFormat));
  appendReceiptField(receipt, "swapchain_color_space", swapchainColorSpaceName(info_.colorSpace));
  appendReceiptField(receipt, "swapchain_present_mode", swapchainPresentModeName(info_.presentMode));
  appendReceiptField(receipt, "swapchain_extent", extentString(info_.extent));
  appendReceiptField(receipt, "swapchain_image_count", static_cast<std::uint64_t>(info_.imageCount));
  appendReceiptField(receipt, "swapchain_transfer_src_supported", info_.transferSourceSupported);
  appendReceiptField(receipt, "swapchain_recreate_count",
                     static_cast<std::uint64_t>(info_.recreateCount));
  appendReceiptField(receipt, "result", result);
  appendReceiptField(receipt, "reason_code", reasonCode);
  return receipt;
}

SwapchainOperationResult Swapchain::create(const SwapchainCreateInfo& createInfo) {
  destroy();
  createInfo_ = createInfo;
  hasCreateInfo_ = true;
  return createInternal(createInfo.drawableWidth, createInfo.drawableHeight, VK_NULL_HANDLE, false);
}

SwapchainOperationResult Swapchain::recreate(std::uint32_t drawableWidth,
                                             std::uint32_t drawableHeight) {
  if (!hasCreateInfo_) {
    SwapchainOperationResult result;
    result.outcome = RenderOutcome::RendererNotReady;
    result.reason = reasonFor("swapchain_recreate_failed");
    result.receipt = makeReceipt("fail", result.reason.code);
    return result;
  }
  return createInternal(drawableWidth, drawableHeight, swapchain_, true);
}

SwapchainOperationResult Swapchain::markDrawableExtent(std::uint32_t width, std::uint32_t height) {
  SwapchainOperationResult result;
  if (width == 0U || height == 0U) {
    info_.state = SwapchainState::NotDrawable;
    result.outcome = RenderOutcome::SkipFrame;
    result.reason = reasonFor("swapchain_not_drawable");
    result.receipt = makeReceipt("skip", result.reason.code);
    return result;
  }
  if (info_.state == SwapchainState::NotDrawable ||
      info_.extent.width != width || info_.extent.height != height) {
    createInfo_.drawableWidth = width;
    createInfo_.drawableHeight = height;
    info_.state = SwapchainState::DirtyResize;
    result.outcome = RenderOutcome::RecreateSwapchain;
    result.reason = reasonFor("swapchain_recreate_requested");
    result.receipt = makeReceipt("skip", result.reason.code);
    return result;
  }
  result.outcome = RenderOutcome::Ok;
  result.reason = reasonFor("vulkan_smoke_pass");
  result.receipt = makeReceipt("pass", result.reason.code);
  return result;
}

SwapchainOperationResult Swapchain::createInternal(std::uint32_t drawableWidth,
                                                   std::uint32_t drawableHeight,
                                                   VkSwapchainKHR oldSwapchain,
                                                   bool recreating) {
  SwapchainOperationResult result;
  if (drawableWidth == 0U || drawableHeight == 0U) {
    info_.state = SwapchainState::NotDrawable;
    result.outcome = RenderOutcome::SkipFrame;
    result.reason = reasonFor("swapchain_not_drawable");
    result.receipt = makeReceipt("skip", result.reason.code);
    return result;
  }

  info_.state = recreating ? SwapchainState::Recreating : SwapchainState::Uninitialized;

  VkSurfaceCapabilitiesKHR capabilities{};
  VkResult vkResult = createInfo_.functions.instance.getPhysicalDeviceSurfaceCapabilitiesKHR(
      createInfo_.physicalDevice, createInfo_.surface, &capabilities);
  if (vkResult != VK_SUCCESS) {
    info_.state = SwapchainState::Failed;
    result.outcome = RenderOutcome::FatalRendererError;
    result.reason = reasonFor("swapchain_create_failed");
    result.receipt = makeReceipt("fail", result.reason.code);
    return result;
  }

  std::uint32_t formatCount = 0;
  vkResult = createInfo_.functions.instance.getPhysicalDeviceSurfaceFormatsKHR(
      createInfo_.physicalDevice, createInfo_.surface, &formatCount, nullptr);
  if (vkResult != VK_SUCCESS || formatCount == 0U) {
    info_.state = SwapchainState::Failed;
    result.outcome = RenderOutcome::Unsupported;
    result.reason = reasonFor("swapchain_create_failed");
    result.receipt = makeReceipt("fail", result.reason.code);
    return result;
  }
  std::vector<VkSurfaceFormatKHR> formats(formatCount);
  createInfo_.functions.instance.getPhysicalDeviceSurfaceFormatsKHR(
      createInfo_.physicalDevice, createInfo_.surface, &formatCount, formats.data());

  std::uint32_t presentModeCount = 0;
  vkResult = createInfo_.functions.instance.getPhysicalDeviceSurfacePresentModesKHR(
      createInfo_.physicalDevice, createInfo_.surface, &presentModeCount, nullptr);
  if (vkResult != VK_SUCCESS) {
    info_.state = SwapchainState::Failed;
    result.outcome = RenderOutcome::Unsupported;
    result.reason = reasonFor("swapchain_create_failed");
    result.receipt = makeReceipt("fail", result.reason.code);
    appendReceiptField(result.receipt, "present_mode_query_result", vkResultName(vkResult));
    return result;
  }
  std::vector<VkPresentModeKHR> presentModes(presentModeCount);
  if (presentModeCount != 0U) {
    vkResult = createInfo_.functions.instance.getPhysicalDeviceSurfacePresentModesKHR(
        createInfo_.physicalDevice, createInfo_.surface, &presentModeCount, presentModes.data());
    if (vkResult != VK_SUCCESS) {
      info_.state = SwapchainState::Failed;
      result.outcome = RenderOutcome::Unsupported;
      result.reason = reasonFor("swapchain_create_failed");
      result.receipt = makeReceipt("fail", result.reason.code);
      appendReceiptField(result.receipt, "present_mode_query_result", vkResultName(vkResult));
      return result;
    }
  }
  if (presentModes.empty()) {
    presentModes.push_back(VK_PRESENT_MODE_FIFO_KHR);
  }

  const VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(formats);
  const VkPresentModeKHR presentMode =
      choosePresentMode(presentModes, createInfo_.config.presentMode);
  const VkExtent2D extent = chooseExtent(capabilities, drawableWidth, drawableHeight);
  const std::uint32_t imageCount = chooseImageCount(capabilities);

  VkSwapchainKHR previousSwapchain = oldSwapchain;
  if (recreating) {
    destroyImageViews();
  }

  std::uint32_t queueFamilyIndices[] = {createInfo_.queues.graphicsFamily,
                                        createInfo_.queues.presentFamily};
  VkSwapchainCreateInfoKHR swapchainInfo{};
  swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapchainInfo.surface = createInfo_.surface;
  swapchainInfo.minImageCount = imageCount;
  swapchainInfo.imageFormat = surfaceFormat.format;
  swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;
  swapchainInfo.imageExtent = extent;
  swapchainInfo.imageArrayLayers = 1;
  swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  if ((capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) != 0U) {
    swapchainInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    info_.transferSourceSupported = true;
  } else {
    info_.transferSourceSupported = false;
  }
  if (createInfo_.queues.graphicsFamily != createInfo_.queues.presentFamily) {
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    swapchainInfo.queueFamilyIndexCount = 2;
    swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }
  swapchainInfo.preTransform = capabilities.currentTransform;
  swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapchainInfo.presentMode = presentMode;
  swapchainInfo.clipped = VK_TRUE;
  swapchainInfo.oldSwapchain = previousSwapchain;

  VkSwapchainKHR newSwapchain = VK_NULL_HANDLE;
  vkResult = createInfo_.functions.device.createSwapchainKHR(createInfo_.device, &swapchainInfo,
                                                             nullptr, &newSwapchain);
  if (vkResult != VK_SUCCESS) {
    info_.state = SwapchainState::Failed;
    result.outcome = mapVkResult(vkResult, VulkanCallContext::Unknown).outcome;
    result.reason = reasonFor(recreating ? "swapchain_recreate_failed" : "swapchain_create_failed");
    result.receipt = makeReceipt("fail", result.reason.code);
    return result;
  }

  if (previousSwapchain != VK_NULL_HANDLE) {
    createInfo_.functions.device.destroySwapchainKHR(createInfo_.device, previousSwapchain,
                                                     nullptr);
  }
  swapchain_ = newSwapchain;

  std::uint32_t actualImageCount = 0;
  vkResult = createInfo_.functions.device.getSwapchainImagesKHR(createInfo_.device, swapchain_,
                                                                &actualImageCount, nullptr);
  if (vkResult != VK_SUCCESS || actualImageCount == 0U) {
    info_.state = SwapchainState::Failed;
    result.outcome = RenderOutcome::FatalRendererError;
    result.reason = reasonFor("swapchain_image_query_failed");
    result.receipt = makeReceipt("fail", result.reason.code);
    return result;
  }
  images_.resize(actualImageCount);
  createInfo_.functions.device.getSwapchainImagesKHR(createInfo_.device, swapchain_,
                                                     &actualImageCount, images_.data());
  imageViews_.clear();
  imageViews_.reserve(images_.size());
  for (VkImage image : images_) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = surfaceFormat.format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    VkImageView imageView = VK_NULL_HANDLE;
    if (vkCreateImageView(createInfo_.device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
      info_.state = SwapchainState::Failed;
      destroyImageViews();
      result.outcome = RenderOutcome::FatalRendererError;
      result.reason = reasonFor("swapchain_image_view_create_failed");
      result.receipt = makeReceipt("fail", result.reason.code);
      return result;
    }
    imageViews_.push_back(imageView);
  }

  info_.colorFormat = surfaceFormat.format;
  info_.colorSpace = surfaceFormat.colorSpace;
  info_.presentMode = presentMode;
  info_.extent = extent;
  info_.imageCount = actualImageCount;
  info_.generation += 1U;
  if (recreating) {
    info_.recreateCount += 1U;
  }
  info_.state = SwapchainState::Ready;
  createInfo_.drawableWidth = drawableWidth;
  createInfo_.drawableHeight = drawableHeight;
  result.outcome = RenderOutcome::Ok;
  result.reason = reasonFor("vulkan_smoke_pass");
  result.receipt = makeReceipt("pass", result.reason.code);
  return result;
}

void Swapchain::destroyImageViews() {
  if (createInfo_.device != VK_NULL_HANDLE) {
    for (VkImageView imageView : imageViews_) {
      if (imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(createInfo_.device, imageView, nullptr);
      }
    }
  }
  imageViews_.clear();
}

void Swapchain::destroy() {
  destroyImageViews();
  if (createInfo_.device != VK_NULL_HANDLE && swapchain_ != VK_NULL_HANDLE &&
      createInfo_.functions.device.destroySwapchainKHR != nullptr) {
    createInfo_.functions.device.destroySwapchainKHR(createInfo_.device, swapchain_, nullptr);
  }
  swapchain_ = VK_NULL_HANDLE;
  images_.clear();
  info_.imageCount = 0;
  info_.state = SwapchainState::Uninitialized;
}

SwapchainAcquireResult Swapchain::acquire(VkSemaphore imageAvailableSemaphore) {
  SwapchainAcquireResult result;
  if (info_.state == SwapchainState::NotDrawable || info_.extent.width == 0U ||
      info_.extent.height == 0U) {
    result.outcome = RenderOutcome::SkipFrame;
    result.reason = reasonFor("swapchain_not_drawable");
    result.receipt = makeReceipt("skip", result.reason.code);
    return result;
  }
  if (!ready()) {
    result.outcome = RenderOutcome::RendererNotReady;
    result.reason = reasonFor("swapchain_acquire_failed");
    result.receipt = makeReceipt("fail", result.reason.code);
    return result;
  }

  std::uint32_t imageIndex = 0;
  const VkResult vkResult = createInfo_.functions.device.acquireNextImageKHR(
      createInfo_.device, swapchain_, std::numeric_limits<std::uint64_t>::max(),
      imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
  result.imageIndex = imageIndex;
  if (vkResult == VK_SUCCESS) {
    result.outcome = RenderOutcome::Ok;
    result.reason = reasonFor("vulkan_smoke_pass");
    result.imageValid = true;
    result.submitAllowed = true;
    result.presentAllowed = true;
    result.receipt = makeReceipt("pass", result.reason.code);
    appendReceiptField(result.receipt, "acquire_result", vkResultName(vkResult));
    appendReceiptField(result.receipt, "acquire_action", "submit");
    appendReceiptField(result.receipt, "acquired_image_index",
                       static_cast<std::uint64_t>(imageIndex));
    return result;
  }
  if (vkResult == VK_SUBOPTIMAL_KHR) {
    info_.state = SwapchainState::DirtySuboptimal;
    result.outcome = RenderOutcome::Ok;
    result.reason = reasonFor("swapchain_suboptimal");
    result.imageValid = true;
    result.submitAllowed = true;
    result.presentAllowed = true;
    result.recreateRequested = true;
    result.receipt = makeReceipt("pass", result.reason.code);
    appendReceiptField(result.receipt, "acquire_result", vkResultName(vkResult));
    appendReceiptField(result.receipt, "acquire_action", "submit_then_recreate");
    appendReceiptField(result.receipt, "acquired_image_index",
                       static_cast<std::uint64_t>(imageIndex));
    return result;
  }
  const VulkanResultMapping mapped = mapVkResult(vkResult, VulkanCallContext::AcquireImage);
  result.outcome = mapped.outcome;
  result.recreateRequested = mapped.outcome == RenderOutcome::RecreateSwapchain ||
                             mapped.reason.code == std::string_view{"swapchain_suboptimal"};
  result.reason = reasonFor(mapped.reason.code);
  if (mapped.reason.code == std::string_view{"swapchain_out_of_date"}) {
    info_.state = SwapchainState::DirtyOutOfDate;
  } else if (mapped.reason.code == std::string_view{"swapchain_suboptimal"}) {
    info_.state = SwapchainState::DirtySuboptimal;
  } else if (mapped.reason.code == std::string_view{"device_lost"}) {
    info_.state = SwapchainState::DeviceLost;
  } else {
    info_.state = SwapchainState::Failed;
    result.reason = reasonFor("swapchain_acquire_failed");
  }
  result.receipt = makeReceipt(result.recreateRequested ? "skip" : "fail", result.reason.code);
  appendReceiptField(result.receipt, "acquire_result", vkResultName(vkResult));
  appendReceiptField(result.receipt, "acquire_action",
                     result.recreateRequested ? "recreate" : "fail");
  appendReceiptField(result.receipt, "acquired_image_index", "none");
  return result;
}

SwapchainPresentResult Swapchain::notePresentResult(VkResult resultCode) {
  SwapchainPresentResult result;
  appendReceiptField(result.receipt, "present_result", vkResultName(resultCode));
  if (resultCode == VK_SUCCESS) {
    result.outcome = RenderOutcome::Ok;
    result.reason = reasonFor("vulkan_smoke_pass");
    result.presented = true;
    result.receipt = makeReceipt("pass", result.reason.code);
    appendReceiptField(result.receipt, "present_result", vkResultName(resultCode));
    appendReceiptField(result.receipt, "present_action", "presented");
    appendReceiptField(result.receipt, "presented", true);
    return result;
  }
  if (resultCode == VK_SUBOPTIMAL_KHR) {
    info_.state = SwapchainState::DirtySuboptimal;
    result.outcome = RenderOutcome::Ok;
    result.reason = reasonFor("swapchain_suboptimal");
    result.presented = true;
    result.recreateRequested = true;
    result.receipt = makeReceipt("pass", result.reason.code);
    appendReceiptField(result.receipt, "present_result", vkResultName(resultCode));
    appendReceiptField(result.receipt, "present_action", "presented_then_recreate");
    appendReceiptField(result.receipt, "presented", true);
    return result;
  }
  const VulkanResultMapping mapped = mapVkResult(resultCode, VulkanCallContext::Present);
  result.outcome = mapped.outcome;
  result.recreateRequested = mapped.outcome == RenderOutcome::RecreateSwapchain ||
                             mapped.reason.code == std::string_view{"swapchain_suboptimal"};
  result.reason = reasonFor(mapped.reason.code);
  if (mapped.reason.code == std::string_view{"swapchain_out_of_date"}) {
    info_.state = SwapchainState::DirtyOutOfDate;
  } else if (mapped.reason.code == std::string_view{"swapchain_suboptimal"}) {
    info_.state = SwapchainState::DirtySuboptimal;
  } else if (mapped.reason.code == std::string_view{"device_lost"}) {
    info_.state = SwapchainState::DeviceLost;
  } else {
    info_.state = SwapchainState::Failed;
    result.reason = reasonFor("swapchain_present_failed");
  }
  result.receipt = makeReceipt(result.recreateRequested ? "skip" : "fail", result.reason.code);
  appendReceiptField(result.receipt, "present_result", vkResultName(resultCode));
  appendReceiptField(result.receipt, "present_action",
                     result.recreateRequested ? "recreate" : "fail");
  appendReceiptField(result.receipt, "presented", false);
  return result;
}

const SwapchainInfo& Swapchain::info() const {
  return info_;
}

VkSwapchainKHR Swapchain::handle() const {
  return swapchain_;
}

VkImage Swapchain::imageAt(std::uint32_t index) const {
  if (index >= images_.size()) {
    return VK_NULL_HANDLE;
  }
  return images_[index];
}

VkImageView Swapchain::imageViewAt(std::uint32_t index) const {
  if (index >= imageViews_.size()) {
    return VK_NULL_HANDLE;
  }
  return imageViews_[index];
}

bool Swapchain::ready() const {
  return info_.state == SwapchainState::Ready && swapchain_ != VK_NULL_HANDLE &&
         !images_.empty() && imageViews_.size() == images_.size();
}

}  // namespace iggy3d::vulkan
