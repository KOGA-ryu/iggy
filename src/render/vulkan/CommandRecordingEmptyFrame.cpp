#include "render/vulkan/CommandRecording.hpp"
#include "render/vulkan/CommandRecordingInternal.hpp"

#include "render/vulkan/VulkanResult.hpp"

namespace iggy3d::vulkan {

using command_recording_internal::recordExternalUi;
using command_recording_internal::recordCaptureAndPresent;
using command_recording_internal::recordHudGlyphQuads;
using command_recording_internal::recordOverlayRects;
using command_recording_internal::reasonFor;

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
  recordOverlayRects(info.commandBuffer, info.uiOverlayRects, info.uiOverlayRectCount);
  recordHudGlyphQuads(info.commandBuffer, info.uiTextGlyphQuads, info.uiTextGlyphQuadCount);
  recordHudGlyphQuads(info.commandBuffer, info.debugHudQuads, info.debugHudQuadCount);
  createInfo_.deviceFunctions.cmdEndRendering(info.commandBuffer);

  const bool externalUiRecorded = recordExternalUi(
      info.externalUiHook, info.commandBuffer, info.swapchainImage,
      info.swapchainImageView, info.extent, info.frameSlot);
  const bool captureRecorded = recordCaptureAndPresent(
      info.commandBuffer, info.swapchainImage, info.extent,
      info.captureEnabled, info.captureBuffer, info.captureBufferSize);

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
  appendReceiptField(result.receipt, "debug_hud_quad_count",
                     static_cast<std::uint64_t>(info.debugHudQuadCount));
  appendReceiptField(result.receipt, "ui_overlay_rect_count",
                     static_cast<std::uint64_t>(info.uiOverlayRectCount));
  appendReceiptField(result.receipt, "ui_text_glyph_quad_count",
                     static_cast<std::uint64_t>(info.uiTextGlyphQuadCount));
  appendReceiptField(result.receipt, "capture_copy_recorded", captureRecorded);
  appendReceiptField(result.receipt, "external_ui_recorded", externalUiRecorded);
  return result;
}

}  // namespace iggy3d::vulkan
