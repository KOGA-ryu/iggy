#pragma once

#include "render/vulkan/CommandRecording.hpp"

namespace iggy3d::vulkan::command_recording_internal {

[[nodiscard]] RenderReason reasonFor(std::string_view code);
[[nodiscard]] VkClearRect clearRect(
    std::int32_t x,
    std::int32_t y,
    std::uint32_t width,
    std::uint32_t height);
[[nodiscard]] VkClearAttachment colorClear(
    float r,
    float g,
    float b,
    float a);
void recordHudGlyphQuads(
    VkCommandBuffer commandBuffer,
    const DebugHudGlyphQuad* quads,
    std::size_t quadCount);
void recordOverlayRects(
    VkCommandBuffer commandBuffer,
    const OverlayRect* rects,
    std::size_t rectCount);
[[nodiscard]] bool recordExternalUi(
    const ExternalUiRecordHook& hook,
    VkCommandBuffer commandBuffer,
    VkImage colorImage,
    VkImageView colorImageView,
    VkExtent2D extent,
    std::uint32_t frameSlot);
[[nodiscard]] bool recordCaptureAndPresent(
    VkCommandBuffer commandBuffer,
    VkImage swapchainImage,
    VkExtent2D extent,
    bool captureEnabled,
    VkBuffer captureBuffer,
    VkDeviceSize captureBufferSize);

}  // namespace iggy3d::vulkan::command_recording_internal
