#include "render/vulkan/CommandRecording.hpp"

#include "render/vulkan/FirstRoomPipeline.hpp"
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

}  // namespace

CreativePreviewCommandPlan buildCreativePreviewCommandPlan(
    const CreativePreviewDrawInfo* draws,
    std::size_t drawCount,
    std::size_t geometryDrawCount) noexcept {
  CreativePreviewCommandPlan plan;
  if (draws == nullptr || drawCount == 0U ||
      drawCount > plan.steps.size()) {
    return plan;
  }
  for (bool depthDisabled : {false, true}) {
    for (std::size_t index = 0; index < drawCount; ++index) {
      if (draws[index].depthDisabled != depthDisabled ||
          draws[index].geometryDrawIndex >= geometryDrawCount) {
        continue;
      }
      plan.steps[plan.stepCount++] = {
          static_cast<std::uint8_t>(index), depthDisabled};
    }
  }
  return plan;
}

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

RenderReceipt firstRoomReceipt(const CommandRecording& recording,
                               std::string_view result,
                               std::string_view reasonCode) {
  (void)recording;
  RenderReceipt receipt;
  appendReceiptField(receipt, "receipt_version", "1");
  appendReceiptField(receipt, "repo", "iggy3d");
  appendReceiptField(receipt, "file_plan", "src/render/vulkan/CommandRecording.cpp");
  appendReceiptField(receipt, "packet_order", "7");
  appendReceiptField(receipt, "backend", "vulkan");
  appendReceiptField(receipt, "record_mode", "first_room");
  appendReceiptField(receipt, "pipeline_family", "first_room");
  appendReceiptField(receipt, "pipeline_variant", kFirstRoomPipelineVariant);
  appendReceiptField(receipt, "depth_enabled", true);
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
  recordOverlayRects(info.commandBuffer, info.uiOverlayRects, info.uiOverlayRectCount);
  recordHudGlyphQuads(info.commandBuffer, info.uiTextGlyphQuads, info.uiTextGlyphQuadCount);
  recordHudGlyphQuads(info.commandBuffer, info.debugHudQuads, info.debugHudQuadCount);
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
  appendReceiptField(result.receipt, "debug_hud_quad_count",
                     static_cast<std::uint64_t>(info.debugHudQuadCount));
  appendReceiptField(result.receipt, "ui_overlay_rect_count",
                     static_cast<std::uint64_t>(info.uiOverlayRectCount));
  appendReceiptField(result.receipt, "ui_text_glyph_quad_count",
                     static_cast<std::uint64_t>(info.uiTextGlyphQuadCount));
  return result;
}

CommandRecordResult CommandRecording::recordFirstRoomFrame(
    const FirstRoomFrameRecordInfo& info) {
  CommandRecordResult result;
  const bool creativePreviewInvalid =
      info.creativePreviewDrawCount > 0U &&
      (info.creativePreviewDrawCount > kRenderCreativePreviewCapacity ||
       info.viewModelPipeline == VK_NULL_HANDLE ||
       info.creativePreviewVertexBuffer == VK_NULL_HANDLE ||
       info.creativePreviewIndexBuffer == VK_NULL_HANDLE ||
       info.creativePreviewIndexedDraws == nullptr ||
       info.creativePreviewIndexedDrawCount == 0U ||
       info.creativePreviewDraws == nullptr);
  if (!ready_ || info.commandBuffer == VK_NULL_HANDLE || info.swapchainImage == VK_NULL_HANDLE ||
      info.swapchainImageView == VK_NULL_HANDLE || info.depthImage == VK_NULL_HANDLE ||
      info.depthImageView == VK_NULL_HANDLE || info.pipeline == VK_NULL_HANDLE ||
      info.pipelineLayout == VK_NULL_HANDLE || info.vertexBuffer == VK_NULL_HANDLE ||
      info.indexBuffer == VK_NULL_HANDLE || info.indexCount == 0U ||
      info.extent.width == 0U || info.extent.height == 0U ||
      creativePreviewInvalid) {
    result.outcome = RenderOutcome::RendererNotReady;
    result.reason = reasonFor("command_record_not_ready");
    result.stage = "precheck";
    result.receipt = firstRoomReceipt(*this, "fail", result.reason.code);
    return result;
  }

  VkResult vkResult = vkResetCommandBuffer(info.commandBuffer, 0);
  if (vkResult != VK_SUCCESS) {
    result.outcome = mapVkResult(vkResult, VulkanCallContext::Unknown).outcome;
    result.reason = reasonFor("command_reset_failed");
    result.stage = "reset";
    result.receipt = firstRoomReceipt(*this, "fail", result.reason.code);
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
    result.receipt = firstRoomReceipt(*this, "fail", result.reason.code);
    return result;
  }

  VkImageMemoryBarrier colorToAttachment{};
  colorToAttachment.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  colorToAttachment.srcAccessMask = 0;
  colorToAttachment.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  colorToAttachment.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorToAttachment.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  colorToAttachment.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  colorToAttachment.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  colorToAttachment.image = info.swapchainImage;
  colorToAttachment.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  colorToAttachment.subresourceRange.baseMipLevel = 0;
  colorToAttachment.subresourceRange.levelCount = 1;
  colorToAttachment.subresourceRange.baseArrayLayer = 0;
  colorToAttachment.subresourceRange.layerCount = 1;

  VkImageMemoryBarrier depthToAttachment{};
  depthToAttachment.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  depthToAttachment.srcAccessMask = 0;
  depthToAttachment.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  depthToAttachment.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthToAttachment.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  depthToAttachment.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  depthToAttachment.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  depthToAttachment.image = info.depthImage;
  depthToAttachment.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  depthToAttachment.subresourceRange.baseMipLevel = 0;
  depthToAttachment.subresourceRange.levelCount = 1;
  depthToAttachment.subresourceRange.baseArrayLayer = 0;
  depthToAttachment.subresourceRange.layerCount = 1;

  VkImageMemoryBarrier barriers[2]{colorToAttachment, depthToAttachment};
  vkCmdPipelineBarrier(info.commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                           VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                       0, 0, nullptr, 0, nullptr, 2, barriers);

  VkClearValue colorClear{};
  colorClear.color.float32[0] = 0.035F;
  colorClear.color.float32[1] = 0.055F;
  colorClear.color.float32[2] = 0.080F;
  colorClear.color.float32[3] = 1.0F;
  VkClearValue depthClear{};
  depthClear.depthStencil.depth = 1.0F;

  VkRenderingAttachmentInfo colorAttachment{};
  colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  colorAttachment.imageView = info.swapchainImageView;
  colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.clearValue = colorClear;

  VkRenderingAttachmentInfo depthAttachment{};
  depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  depthAttachment.imageView = info.depthImageView;
  depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.clearValue = depthClear;

  VkRenderingInfo renderingInfo{};
  renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  renderingInfo.renderArea.offset = {0, 0};
  renderingInfo.renderArea.extent = info.extent;
  renderingInfo.layerCount = 1;
  renderingInfo.colorAttachmentCount = 1;
  renderingInfo.pColorAttachments = &colorAttachment;
  renderingInfo.pDepthAttachment = &depthAttachment;

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
  vkCmdBindPipeline(info.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, info.pipeline);
  VkDeviceSize vertexOffset = 0;
  vkCmdBindVertexBuffers(info.commandBuffer, 0, 1, &info.vertexBuffer, &vertexOffset);
  vkCmdBindIndexBuffer(info.commandBuffer, info.indexBuffer, 0, VK_INDEX_TYPE_UINT16);
  std::uint32_t indexedDrawCount = 0;
  if (info.indexedDraws != nullptr && info.indexedDrawCount > 0U) {
    bool materialPipelineBound = false;
    std::uint32_t boundTextureIndex = kInvalidMaterialTextureIndex;
    const bool materialPipelineReady =
        info.materialTexturePipeline != VK_NULL_HANDLE &&
        info.materialTexturePipelineLayout != VK_NULL_HANDLE &&
        info.materialTextureDescriptorSets != nullptr &&
        info.materialTextureDescriptorSetCount > 0U;
    for (std::size_t drawIndex = 0; drawIndex < info.indexedDrawCount; ++drawIndex) {
      const IndexedDrawRange& draw = info.indexedDraws[drawIndex];
      if (draw.indexCount == 0U) {
        continue;
      }
      const RoomDrawPipelineSelection selection = selectRoomDrawPipeline(
          draw, info.materialTextureDescriptorSetCount,
          materialPipelineReady);
      if (selection.textured != materialPipelineBound) {
        vkCmdBindPipeline(
            info.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            selection.textured ? info.materialTexturePipeline : info.pipeline);
        materialPipelineBound = selection.textured;
        boundTextureIndex = kInvalidMaterialTextureIndex;
      }
      const VkPipelineLayout activeLayout =
          selection.textured ? info.materialTexturePipelineLayout
                             : info.pipelineLayout;
      if (selection.textured && selection.textureIndex != boundTextureIndex) {
        const VkDescriptorSet descriptorSet =
            info.materialTextureDescriptorSets[selection.textureIndex];
        vkCmdBindDescriptorSets(
            info.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            activeLayout, 0U, 1U, &descriptorSet, 0U, nullptr);
        boundTextureIndex = selection.textureIndex;
      }
      vkCmdPushConstants(info.commandBuffer, activeLayout,
                         VK_SHADER_STAGE_VERTEX_BIT, 0,
                         sizeof(FirstRoomPushConstants),
                         &info.pushConstants);
      vkCmdDrawIndexed(info.commandBuffer, draw.indexCount, 1, draw.firstIndex, 0, 0);
      ++indexedDrawCount;
    }
  }
  if (indexedDrawCount == 0U) {
    vkCmdBindPipeline(info.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      info.pipeline);
    vkCmdPushConstants(info.commandBuffer, info.pipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(FirstRoomPushConstants), &info.pushConstants);
    vkCmdDrawIndexed(info.commandBuffer, info.indexCount, 1, 0, 0, 0);
    indexedDrawCount = 1U;
  }
  std::uint32_t creativePreviewDrawCount = 0U;
  if (info.creativePreviewDrawCount > 0U) {
    vkCmdBindVertexBuffers(info.commandBuffer, 0, 1,
                           &info.creativePreviewVertexBuffer, &vertexOffset);
    vkCmdBindIndexBuffer(info.commandBuffer,
                         info.creativePreviewIndexBuffer, 0,
                         VK_INDEX_TYPE_UINT16);
    const CreativePreviewCommandPlan plan = buildCreativePreviewCommandPlan(
        info.creativePreviewDraws, info.creativePreviewDrawCount,
        info.creativePreviewIndexedDrawCount);
    bool pipelineBound = false;
    bool boundDepthDisabled = false;
    for (std::size_t stepIndex = 0; stepIndex < plan.stepCount; ++stepIndex) {
      const CreativePreviewCommandStep& step = plan.steps[stepIndex];
      const CreativePreviewDrawInfo& preview =
          info.creativePreviewDraws[step.sourceDrawIndex];
      if (!pipelineBound || boundDepthDisabled != step.depthDisabled) {
        const VkPipeline previewPipeline =
            step.depthDisabled ? info.viewModelPipeline : info.pipeline;
        vkCmdBindPipeline(info.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          previewPipeline);
        pipelineBound = true;
        boundDepthDisabled = step.depthDisabled;
      }
      const IndexedDrawRange& draw =
          info.creativePreviewIndexedDraws[preview.geometryDrawIndex];
      if (draw.indexCount == 0U) {
        continue;
      }
      vkCmdPushConstants(
          info.commandBuffer, info.pipelineLayout,
          VK_SHADER_STAGE_VERTEX_BIT, 0,
          sizeof(FirstRoomPushConstants), &preview.pushConstants);
      vkCmdDrawIndexed(info.commandBuffer, draw.indexCount, 1,
                       draw.firstIndex, 0, 0);
      ++creativePreviewDrawCount;
    }
  }
  recordOverlayRects(info.commandBuffer, info.projectileOverlayRects,
                     info.projectileOverlayRectCount);
  recordOverlayRects(info.commandBuffer, info.uiOverlayRects,
                     info.uiOverlayRectCount);
  recordHudGlyphQuads(info.commandBuffer, info.uiTextGlyphQuads,
                      info.uiTextGlyphQuadCount);
  recordHudGlyphQuads(info.commandBuffer, info.debugHudQuads, info.debugHudQuadCount);
  createInfo_.deviceFunctions.cmdEndRendering(info.commandBuffer);

  if (info.captureEnabled && info.captureBuffer != VK_NULL_HANDLE &&
      info.captureBufferSize >= static_cast<VkDeviceSize>(info.extent.width) *
                                    info.extent.height * 4ULL) {
    VkImageMemoryBarrier colorToTransfer{};
    colorToTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    colorToTransfer.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    colorToTransfer.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    colorToTransfer.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorToTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    colorToTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    colorToTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    colorToTransfer.image = info.swapchainImage;
    colorToTransfer.subresourceRange = colorToAttachment.subresourceRange;
    vkCmdPipelineBarrier(info.commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &colorToTransfer);

    VkBufferImageCopy copyRegion{};
    copyRegion.bufferOffset = 0;
    copyRegion.bufferRowLength = 0;
    copyRegion.bufferImageHeight = 0;
    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.mipLevel = 0;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount = 1;
    copyRegion.imageExtent = {info.extent.width, info.extent.height, 1U};
    vkCmdCopyImageToBuffer(info.commandBuffer, info.swapchainImage,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, info.captureBuffer, 1,
                           &copyRegion);

    VkImageMemoryBarrier transferToPresent{};
    transferToPresent.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    transferToPresent.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    transferToPresent.dstAccessMask = 0;
    transferToPresent.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    transferToPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    transferToPresent.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    transferToPresent.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    transferToPresent.image = info.swapchainImage;
    transferToPresent.subresourceRange = colorToAttachment.subresourceRange;
    vkCmdPipelineBarrier(info.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &transferToPresent);
  } else {
    VkImageMemoryBarrier colorToPresent{};
    colorToPresent.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    colorToPresent.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    colorToPresent.dstAccessMask = 0;
    colorToPresent.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorToPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    colorToPresent.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    colorToPresent.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    colorToPresent.image = info.swapchainImage;
    colorToPresent.subresourceRange = colorToAttachment.subresourceRange;
    vkCmdPipelineBarrier(info.commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &colorToPresent);
  }

  vkResult = vkEndCommandBuffer(info.commandBuffer);
  if (vkResult != VK_SUCCESS) {
    result.outcome = mapVkResult(vkResult, VulkanCallContext::Unknown).outcome;
    result.reason = reasonFor("command_end_failed");
    result.stage = "end";
    result.receipt = firstRoomReceipt(*this, "fail", result.reason.code);
    return result;
  }

  result.outcome = RenderOutcome::Ok;
  result.reason = reasonFor("first_room_frame_recorded");
  result.recorded = true;
  result.stage = "first_room";
  result.receipt = firstRoomReceipt(*this, "pass", result.reason.code);
  appendReceiptField(result.receipt, "dynamic_rendering_begin", true);
  appendReceiptField(result.receipt, "dynamic_rendering_end", true);
  appendReceiptField(result.receipt, "command_recorded", true);
  appendReceiptField(result.receipt, "frame_slot", static_cast<std::uint64_t>(info.frameSlot));
  appendReceiptField(result.receipt, "swapchain_image_index",
                     static_cast<std::uint64_t>(info.imageIndex));
  appendReceiptField(result.receipt, "draw_count", static_cast<std::uint64_t>(indexedDrawCount));
  appendReceiptField(result.receipt, "indexed_draw_count",
                     static_cast<std::uint64_t>(indexedDrawCount));
  appendReceiptField(result.receipt, "creative_preview_draw_count",
                     static_cast<std::uint64_t>(creativePreviewDrawCount));
  appendReceiptField(result.receipt, "index_count", static_cast<std::uint64_t>(info.indexCount));
  appendReceiptField(result.receipt, "ui_overlay_rect_count",
                     static_cast<std::uint64_t>(info.uiOverlayRectCount));
  appendReceiptField(result.receipt, "ui_text_glyph_quad_count",
                     static_cast<std::uint64_t>(info.uiTextGlyphQuadCount));
  appendReceiptField(result.receipt, "capture_copy_recorded",
                     info.captureEnabled && info.captureBuffer != VK_NULL_HANDLE);
  return result;
}

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
  return result;
}

bool CommandRecording::ready() const {
  return ready_;
}

}  // namespace iggy3d::vulkan
