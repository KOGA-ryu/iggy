#include "render/vulkan/CommandRecording.hpp"

#include "render/vulkan/VulkanResult.hpp"

namespace iggy3d::vulkan {
namespace {

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

}  // namespace

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

CommandRecordResult CommandRecording::recordEmptyFrame(const EmptyFrameRecordInfo& info) {
  CommandRecordResult result;
  if (!ready_ || info.commandBuffer == VK_NULL_HANDLE || info.swapchainImage == VK_NULL_HANDLE ||
      info.swapchainImageView == VK_NULL_HANDLE || info.extent.width == 0U ||
      info.extent.height == 0U) {
    result.outcome = RenderOutcome::RendererNotReady;
    result.reason = reasonFor("command_record_not_ready");
    result.stage = "precheck";
    result.receipt = diagnostics("fail", result.reason.code);
    return result;
  }

  VkResult vkResult = vkResetCommandBuffer(info.commandBuffer, 0);
  if (vkResult != VK_SUCCESS) {
    result.outcome = mapVkResult(vkResult, VulkanCallContext::Unknown).outcome;
    result.reason = reasonFor("command_reset_failed");
    result.stage = "reset";
    result.receipt = diagnostics("fail", result.reason.code);
    return result;
  }

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkResult = vkBeginCommandBuffer(info.commandBuffer, &beginInfo);
  if (vkResult != VK_SUCCESS) {
    result.outcome = mapVkResult(vkResult, VulkanCallContext::Unknown).outcome;
    result.reason = reasonFor("command_begin_failed");
    result.stage = "begin";
    result.receipt = diagnostics("fail", result.reason.code);
    return result;
  }

  VkImageMemoryBarrier toColor{};
  toColor.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  toColor.srcAccessMask = 0;
  toColor.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  toColor.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  toColor.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  toColor.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  toColor.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  toColor.image = info.swapchainImage;
  toColor.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  toColor.subresourceRange.baseMipLevel = 0;
  toColor.subresourceRange.levelCount = 1;
  toColor.subresourceRange.baseArrayLayer = 0;
  toColor.subresourceRange.layerCount = 1;
  vkCmdPipelineBarrier(info.commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &toColor);

  VkClearValue clearValue{};
  clearValue.color.float32[0] = info.clearR;
  clearValue.color.float32[1] = info.clearG;
  clearValue.color.float32[2] = info.clearB;
  clearValue.color.float32[3] = info.clearA;

  VkRenderingAttachmentInfo colorAttachment{};
  colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  colorAttachment.imageView = info.swapchainImageView;
  colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.clearValue = clearValue;

  VkRenderingInfo renderingInfo{};
  renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  renderingInfo.renderArea.offset = {0, 0};
  renderingInfo.renderArea.extent = info.extent;
  renderingInfo.layerCount = 1;
  renderingInfo.colorAttachmentCount = 1;
  renderingInfo.pColorAttachments = &colorAttachment;

  VkViewport viewport{};
  viewport.x = 0.0F;
  viewport.y = 0.0F;
  viewport.width = static_cast<float>(info.extent.width);
  viewport.height = static_cast<float>(info.extent.height);
  viewport.minDepth = 0.0F;
  viewport.maxDepth = 1.0F;
  VkRect2D scissor{};
  scissor.extent = info.extent;
  vkCmdSetViewport(info.commandBuffer, 0, 1, &viewport);
  vkCmdSetScissor(info.commandBuffer, 0, 1, &scissor);
  createInfo_.deviceFunctions.cmdBeginRendering(info.commandBuffer, &renderingInfo);
  createInfo_.deviceFunctions.cmdEndRendering(info.commandBuffer);

  VkImageMemoryBarrier toPresent{};
  toPresent.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  toPresent.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  toPresent.dstAccessMask = 0;
  toPresent.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  toPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  toPresent.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  toPresent.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  toPresent.image = info.swapchainImage;
  toPresent.subresourceRange = toColor.subresourceRange;
  vkCmdPipelineBarrier(info.commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1,
                       &toPresent);

  vkResult = vkEndCommandBuffer(info.commandBuffer);
  if (vkResult != VK_SUCCESS) {
    result.outcome = mapVkResult(vkResult, VulkanCallContext::Unknown).outcome;
    result.reason = reasonFor("command_end_failed");
    result.stage = "end";
    result.receipt = diagnostics("fail", result.reason.code);
    return result;
  }

  result.outcome = RenderOutcome::Ok;
  result.reason = reasonFor("vulkan_smoke_pass");
  result.recorded = true;
  result.stage = "empty_frame";
  result.receipt = diagnostics("pass", result.reason.code);
  appendReceiptField(result.receipt, "dynamic_rendering_begin", true);
  appendReceiptField(result.receipt, "dynamic_rendering_end", true);
  appendReceiptField(result.receipt, "command_recorded", true);
  appendReceiptField(result.receipt, "frame_slot", static_cast<std::uint64_t>(info.frameSlot));
  appendReceiptField(result.receipt, "swapchain_image_index",
                     static_cast<std::uint64_t>(info.imageIndex));
  return result;
}

bool CommandRecording::ready() const {
  return ready_;
}

}  // namespace iggy3d::vulkan
