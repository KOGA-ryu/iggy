#pragma once

#include "render/vulkan/RenderLoopFramePlan.hpp"
#include "render/vulkan/RenderLoopSubmission.hpp"

namespace iggy3d::vulkan {

void appendRenderLoopFrameReceipt(
    RenderReceipt& receipt, const VulkanFrameResult& result,
    const RenderLoopCreateInfo& createInfo, const FrameInput& frame,
    const SwapchainInfo& readySwapchain,
    const RenderLoopFramePlan& framePlan,
    const SwapchainAcquireResult& acquire,
    const RenderLoopSubmissionResult& submission);

}  // namespace iggy3d::vulkan
