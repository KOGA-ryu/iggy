#include "render/vulkan/CommandRecording.hpp"
#include "render/vulkan/CommandRecordingInternal.hpp"

#include "render/vulkan/VulkanResult.hpp"

namespace iggy3d::vulkan {

using command_recording_internal::clearRect;
using command_recording_internal::colorClear;
using command_recording_internal::recordExternalUi;
using command_recording_internal::recordHudGlyphQuads;
using command_recording_internal::recordOverlayRects;
using command_recording_internal::reasonFor;

CommandRecordResult CommandRecording::recordProxyPrimitiveFrame(
    const ProxyPrimitiveFrameRecordInfo& info) {
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
  clearValue.color.float32[0] = 0.035F;
  clearValue.color.float32[1] = 0.055F;
  clearValue.color.float32[2] = 0.080F;
  clearValue.color.float32[3] = 1.0F;

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
  std::uint32_t drawCount = 0;
  const std::uint32_t width = info.extent.width;
  const std::uint32_t height = info.extent.height;
  if (info.floorVisible) {
    const VkClearAttachment floor = colorClear(0.09F, 0.16F, 0.11F, 1.0F);
    const VkClearRect rect = clearRect(0, static_cast<std::int32_t>(height / 2U), width,
                                      height - (height / 2U));
    vkCmdClearAttachments(info.commandBuffer, 1, &floor, 1, &rect);
    ++drawCount;
  }
  if (info.roomBoundsVisible) {
    const VkClearAttachment bounds = colorClear(0.45F, 0.55F, 0.70F, 1.0F);
    const std::uint32_t thickness = width > 900U ? 8U : 4U;
    const VkClearRect rects[4] = {
        clearRect(0, 0, width, thickness),
        clearRect(0, static_cast<std::int32_t>(height - thickness), width, thickness),
        clearRect(0, 0, thickness, height),
        clearRect(static_cast<std::int32_t>(width - thickness), 0, thickness, height)};
    vkCmdClearAttachments(info.commandBuffer, 1, &bounds, 4, rects);
    ++drawCount;
  }
  if (info.playerMarkerVisible) {
    const VkClearAttachment player = colorClear(0.10F, 0.72F, 0.95F, 1.0F);
    const std::uint32_t size = height > 900U ? 32U : 18U;
    const VkClearRect rect = clearRect(static_cast<std::int32_t>((width - size) / 2U),
                                      static_cast<std::int32_t>((height - size) / 2U),
                                      size, size);
    vkCmdClearAttachments(info.commandBuffer, 1, &player, 1, &rect);
    ++drawCount;
  }
  if (info.targetMarkerVisible) {
    const VkClearAttachment target = colorClear(0.92F, 0.20F, 0.18F, 1.0F);
    const std::uint32_t markerWidth = width > 900U ? 44U : 26U;
    const std::uint32_t markerHeight = height > 900U ? 72U : 42U;
    const VkClearRect rect =
        clearRect(static_cast<std::int32_t>((width * 3U / 4U) - (markerWidth / 2U)),
                  static_cast<std::int32_t>((height / 2U) - markerHeight),
                  markerWidth, markerHeight);
    vkCmdClearAttachments(info.commandBuffer, 1, &target, 1, &rect);
    ++drawCount;
  }
  if (info.objectiveMarkerVisible) {
    const VkClearAttachment objective = colorClear(0.95F, 0.78F, 0.18F, 1.0F);
    const std::uint32_t size = height > 900U ? 36U : 22U;
    const VkClearRect rect =
        clearRect(static_cast<std::int32_t>((width / 4U) - (size / 2U)),
                  static_cast<std::int32_t>((height / 2U) - size),
                  size, size);
    vkCmdClearAttachments(info.commandBuffer, 1, &objective, 1, &rect);
    ++drawCount;
  }
  recordOverlayRects(info.commandBuffer, info.projectileOverlayRects,
                     info.projectileOverlayRectCount);
  recordHudGlyphQuads(info.commandBuffer, info.debugHudQuads, info.debugHudQuadCount);
  createInfo_.deviceFunctions.cmdEndRendering(info.commandBuffer);

  const bool externalUiRecorded = recordExternalUi(
      info.externalUiHook, info.commandBuffer, info.swapchainImage,
      info.swapchainImageView, info.extent, info.frameSlot);

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
  result.reason = reasonFor("proxy_primitives_recorded");
  result.recorded = true;
  result.stage = "proxy_primitives";
  result.receipt = diagnostics("pass", result.reason.code);
  appendReceiptField(result.receipt, "record_mode", "draw_primitives");
  appendReceiptField(result.receipt, "dynamic_rendering_begin", true);
  appendReceiptField(result.receipt, "dynamic_rendering_end", true);
  appendReceiptField(result.receipt, "command_recorded", true);
  appendReceiptField(result.receipt, "frame_slot", static_cast<std::uint64_t>(info.frameSlot));
  appendReceiptField(result.receipt, "swapchain_image_index",
                     static_cast<std::uint64_t>(info.imageIndex));
  appendReceiptField(result.receipt, "draw_count", static_cast<std::uint64_t>(drawCount));
  appendReceiptField(result.receipt, "projectile_overlay_rect_count",
                     static_cast<std::uint64_t>(info.projectileOverlayRectCount));
  appendReceiptField(result.receipt, "external_ui_recorded", externalUiRecorded);
  return result;
}

}  // namespace iggy3d::vulkan
