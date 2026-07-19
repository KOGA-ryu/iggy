#include "render/vulkan/CommandRecording.hpp"
#include "render/vulkan/CommandRecordingInternal.hpp"

#include <algorithm>

#include "render/vulkan/VulkanResult.hpp"

namespace iggy3d::vulkan {
namespace command_recording_internal {

RenderReason reasonFor(std::string_view code) {
  if (code == "vulkan_smoke_pass") {
    return {code, "vulkan smoke pass"};
  }
  if (code == "command_pool_create_failed") {
    return {code, "command pool create failed"};
  }
  if (code == "command_buffer_allocate_failed") {
    return {code, "command buffer allocate failed"};
  }
  if (code == "command_reset_failed") {
    return {code, "command reset failed"};
  }
  if (code == "command_begin_failed") {
    return {code, "command begin failed"};
  }
  if (code == "command_end_failed") {
    return {code, "command end failed"};
  }
  if (code == "command_record_failed") {
    return {code, "command record failed"};
  }
  if (code == "first_room_frame_recorded") {
    return {code, "first room frame recorded"};
  }
  if (code == "proxy_primitives_recorded") {
    return {code, "proxy primitives recorded"};
  }
  if (code == "dynamic_rendering_function_missing") {
    return {code, "dynamic rendering function missing"};
  }
  if (code == "image_layout_missing_swapchain_to_color") {
    return {code, "image layout missing swapchain to color"};
  }
  if (code == "image_layout_missing_color_to_present") {
    return {code, "image layout missing color to present"};
  }
  return {"command_record_not_ready", "command recording not ready"};
}

VkClearRect clearRect(std::int32_t x,
                      std::int32_t y,
                      std::uint32_t width,
                      std::uint32_t height) {
  VkClearRect rect{};
  rect.rect.offset = {x, y};
  rect.rect.extent = {width, height};
  rect.baseArrayLayer = 0;
  rect.layerCount = 1;
  return rect;
}

VkClearAttachment colorClear(float r, float g, float b, float a) {
  VkClearAttachment clear{};
  clear.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  clear.colorAttachment = 0;
  clear.clearValue.color.float32[0] = r;
  clear.clearValue.color.float32[1] = g;
  clear.clearValue.color.float32[2] = b;
  clear.clearValue.color.float32[3] = a;
  return clear;
}

void recordHudGlyphQuads(VkCommandBuffer commandBuffer,
                         const DebugHudGlyphQuad* quads,
                         std::size_t quadCount) {
  if (quads == nullptr || quadCount == 0U) {
    return;
  }
  for (std::size_t index = 0; index < quadCount; ++index) {
    const DebugHudGlyphQuad& quad = quads[index];
    if (quad.width == 0U || quad.height == 0U) {
      continue;
    }
    const VkClearAttachment glyph = colorClear(quad.r, quad.g, quad.b, quad.a);
    const VkClearRect rect = clearRect(quad.x, quad.y, quad.width, quad.height);
    vkCmdClearAttachments(commandBuffer, 1, &glyph, 1, &rect);
  }
}

void recordOverlayRects(VkCommandBuffer commandBuffer,
                        const OverlayRect* rects,
                        std::size_t rectCount) {
  if (rects == nullptr || rectCount == 0U) {
    return;
  }
  for (std::size_t index = 0; index < rectCount; ++index) {
    const OverlayRect& overlay = rects[index];
    if (overlay.width == 0U || overlay.height == 0U) {
      continue;
    }
    const VkClearAttachment clear =
        colorClear(overlay.r, overlay.g, overlay.b, overlay.a);
    const VkClearRect rect =
        clearRect(overlay.x, overlay.y, overlay.width, overlay.height);
    vkCmdClearAttachments(commandBuffer, 1, &clear, 1, &rect);
  }
}

// The frame's rendering block has ended and the color image is still in
// COLOR_ATTACHMENT_OPTIMAL; the hook owner records its own UI pass before
// the capture-copy/present barriers.
bool recordExternalUi(const ExternalUiRecordHook& hook,
                      VkCommandBuffer commandBuffer,
                      VkImage colorImage,
                      VkImageView colorImageView,
                      VkExtent2D extent,
                      std::uint32_t frameSlot) {
  if (hook.record == nullptr) {
    return false;
  }
  ExternalUiRecordTarget target;
  target.commandBuffer = commandBuffer;
  target.colorImage = colorImage;
  target.colorImageView = colorImageView;
  target.extent = extent;
  target.frameSlot = frameSlot;
  return hook.record(hook.user, target);
}

}  // namespace command_recording_internal

using command_recording_internal::recordExternalUi;
using command_recording_internal::recordHudGlyphQuads;
using command_recording_internal::recordOverlayRects;
using command_recording_internal::reasonFor;

CommandRecording::~CommandRecording() {
  destroy();
}

RenderReceipt CommandRecording::diagnostics(std::string_view result,
                                            std::string_view reasonCode) const {
  RenderReceipt receipt;
  appendReceiptField(receipt, "receipt_version", "1");
  appendReceiptField(receipt, "repo", "iggy3d");
  appendReceiptField(receipt, "file_plan", "src/render/vulkan/CommandRecording.cpp");
  appendReceiptField(receipt, "packet_order", "5");
  appendReceiptField(receipt, "backend", "vulkan");
  appendReceiptField(receipt, "command_pool_created", commandPool_ != VK_NULL_HANDLE);
  appendReceiptField(receipt, "command_buffer_count",
                     static_cast<std::uint64_t>(commandBuffers_.size()));
  appendReceiptField(receipt, "record_mode", "empty_frame");
  appendReceiptField(receipt, "begin_rendering_function_loaded",
                     createInfo_.deviceFunctions.cmdBeginRendering != nullptr);
  appendReceiptField(receipt, "end_rendering_function_loaded",
                     createInfo_.deviceFunctions.cmdEndRendering != nullptr);
  appendReceiptField(receipt, "result", result);
  appendReceiptField(receipt, "reason_code", reasonCode);
  return receipt;
}

CommandRecordResult CommandRecording::create(const CommandRecordingCreateInfo& createInfo) {
  destroy();
  createInfo_ = createInfo;
  CommandRecordResult result;

  if (createInfo_.deviceFunctions.cmdBeginRendering == nullptr ||
      createInfo_.deviceFunctions.cmdEndRendering == nullptr) {
    result.outcome = RenderOutcome::RendererNotReady;
    result.reason = reasonFor("dynamic_rendering_function_missing");
    result.stage = "dynamic_rendering_functions";
    result.receipt = diagnostics("fail", result.reason.code);
    return result;
  }

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = createInfo_.graphicsQueueFamily;
  if (vkCreateCommandPool(createInfo_.device, &poolInfo, nullptr, &commandPool_) != VK_SUCCESS) {
    result.outcome = RenderOutcome::FatalRendererError;
    result.reason = reasonFor("command_pool_create_failed");
    result.stage = "command_pool";
    result.receipt = diagnostics("fail", result.reason.code);
    return result;
  }

  commandBuffers_.resize(createInfo_.frameSlotCount);
  VkCommandBufferAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocateInfo.commandPool = commandPool_;
  allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocateInfo.commandBufferCount = static_cast<std::uint32_t>(commandBuffers_.size());
  if (vkAllocateCommandBuffers(createInfo_.device, &allocateInfo, commandBuffers_.data()) !=
      VK_SUCCESS) {
    result.outcome = RenderOutcome::FatalRendererError;
    result.reason = reasonFor("command_buffer_allocate_failed");
    result.stage = "command_buffer_allocate";
    destroy();
    result.receipt = diagnostics("fail", result.reason.code);
    return result;
  }

  ready_ = true;
  result.outcome = RenderOutcome::Ok;
  result.reason = reasonFor("vulkan_smoke_pass");
  result.stage = "create";
  result.receipt = diagnostics("pass", result.reason.code);
  return result;
}

void CommandRecording::destroy() {
  if (createInfo_.device != VK_NULL_HANDLE && commandPool_ != VK_NULL_HANDLE) {
    vkDestroyCommandPool(createInfo_.device, commandPool_, nullptr);
  }
  commandPool_ = VK_NULL_HANDLE;
  commandBuffers_.clear();
  ready_ = false;
}

VkCommandBuffer CommandRecording::commandBufferForFrameSlot(std::uint32_t frameSlot) const {
  if (frameSlot >= commandBuffers_.size()) {
    return VK_NULL_HANDLE;
  }
  return commandBuffers_[frameSlot];
}

bool CommandRecording::ready() const {
  return ready_;
}

}  // namespace iggy3d::vulkan
