#pragma once

#include <cstdint>

#include "render/vulkan/RenderLoop.hpp"

namespace iggy3d::vulkan {

enum class RenderLoopSubmissionStage : std::uint8_t {
  ResetFailed,
  SubmitFailed,
  PresentComplete,
};

struct RenderLoopSubmissionResult {
  RenderLoopSubmissionStage stage = RenderLoopSubmissionStage::ResetFailed;
  FrameSyncOperationResult reset;
  VkResult submitResult = VK_SUCCESS;
  VkResult presentResult = VK_SUCCESS;
  SwapchainPresentResult presentation;
};

RenderLoopSubmissionResult submitAndPresentRenderLoopFrame(
    const RenderLoopCreateInfo& createInfo,
    const FrameSyncSubmitPlan& submitPlan, VkCommandBuffer commandBuffer,
    std::uint32_t imageIndex);

}  // namespace iggy3d::vulkan
