#pragma once

#include <cstdint>

#include "render/vulkan/RenderLoopFramePlan.hpp"

namespace iggy3d::vulkan {

struct RenderLoopCommandRecord {
  VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
  CommandRecordResult result;
};

RenderLoopCommandRecord recordRenderLoopFrameCommands(
    const RenderLoopCreateInfo& createInfo, const FrameInput& frame,
    const RenderLoopFramePlan& framePlan, const SwapchainInfo& readySwapchain,
    std::uint32_t frameSlot, std::uint32_t imageIndex);

}  // namespace iggy3d::vulkan
