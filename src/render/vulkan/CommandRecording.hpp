#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "render/RenderDiagnostics.hpp"
#include "render/debug/DebugHudText.hpp"
#include "render/vulkan/FirstRoomPipeline.hpp"
#include "render/vulkan/PipelineLayout.hpp"
#include "render/vulkan/VulkanFunctions.hpp"
#include "render/vulkan/VulkanTypes.hpp"

namespace iggy3d::vulkan {

struct CommandRecordingCreateInfo {
  VkDevice device{};
  VulkanDeviceFunctions deviceFunctions;
  std::uint32_t graphicsQueueFamily = kInvalidVulkanQueueFamily;
  std::uint32_t frameSlotCount = 2;
};

struct OverlayRect {
  std::int32_t x = 0;
  std::int32_t y = 0;
  std::uint32_t width = 0;
  std::uint32_t height = 0;
  float r = 1.0F;
  float g = 1.0F;
  float b = 1.0F;
  float a = 1.0F;
};

struct EmptyFrameRecordInfo {
  VkCommandBuffer commandBuffer{};
  VkImage swapchainImage{};
  VkImageView swapchainImageView{};
  VkFormat colorFormat{};
  VkExtent2D extent{};
  std::uint32_t frameSlot = 0;
  std::uint32_t imageIndex = 0;
  float clearR = 0.035F;
  float clearG = 0.055F;
  float clearB = 0.080F;
  float clearA = 1.0F;
  const OverlayRect* uiOverlayRects = nullptr;
  std::size_t uiOverlayRectCount = 0;
  const DebugHudGlyphQuad* uiTextGlyphQuads = nullptr;
  std::size_t uiTextGlyphQuadCount = 0;
  const DebugHudGlyphQuad* debugHudQuads = nullptr;
  std::size_t debugHudQuadCount = 0;
};

struct FirstRoomFrameRecordInfo {
  VkCommandBuffer commandBuffer{};
  VkImage swapchainImage{};
  VkImageView swapchainImageView{};
  VkFormat colorFormat{};
  VkImage depthImage{};
  VkImageView depthImageView{};
  VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
  VkExtent2D extent{};
  std::uint32_t frameSlot = 0;
  std::uint32_t imageIndex = 0;
  VkPipeline pipeline{};
  VkPipelineLayout pipelineLayout{};
  VkBuffer vertexBuffer{};
  VkBuffer indexBuffer{};
  std::uint32_t indexCount = 0;
  const IndexedDrawRange* indexedDraws = nullptr;
  std::size_t indexedDrawCount = 0;
  FirstRoomPushConstants pushConstants;
  bool captureEnabled = false;
  VkBuffer captureBuffer{};
  VkDeviceSize captureBufferSize = 0;
  const OverlayRect* projectileOverlayRects = nullptr;
  std::size_t projectileOverlayRectCount = 0;
  const DebugHudGlyphQuad* debugHudQuads = nullptr;
  std::size_t debugHudQuadCount = 0;
};

struct ProxyPrimitiveFrameRecordInfo {
  VkCommandBuffer commandBuffer{};
  VkImage swapchainImage{};
  VkImageView swapchainImageView{};
  VkFormat colorFormat{};
  VkExtent2D extent{};
  std::uint32_t frameSlot = 0;
  std::uint32_t imageIndex = 0;
  bool floorVisible = true;
  bool roomBoundsVisible = true;
  bool playerMarkerVisible = true;
  bool targetMarkerVisible = false;
  bool objectiveMarkerVisible = false;
  const OverlayRect* projectileOverlayRects = nullptr;
  std::size_t projectileOverlayRectCount = 0;
  const DebugHudGlyphQuad* debugHudQuads = nullptr;
  std::size_t debugHudQuadCount = 0;
};

struct CommandRecordResult {
  RenderOutcome outcome = RenderOutcome::RendererNotReady;
  RenderReason reason{"command_record_not_ready", "command recording not ready"};
  bool recorded = false;
  const char* stage = "not_ready";
  RenderReceipt receipt;
};

class CommandRecording {
public:
  CommandRecording() = default;
  ~CommandRecording();

  CommandRecording(const CommandRecording&) = delete;
  CommandRecording& operator=(const CommandRecording&) = delete;

  CommandRecordResult create(const CommandRecordingCreateInfo& createInfo);
  void destroy();

  VkCommandBuffer commandBufferForFrameSlot(std::uint32_t frameSlot) const;
  CommandRecordResult recordEmptyFrame(const EmptyFrameRecordInfo& info);
  CommandRecordResult recordFirstRoomFrame(const FirstRoomFrameRecordInfo& info);
  CommandRecordResult recordProxyPrimitiveFrame(const ProxyPrimitiveFrameRecordInfo& info);
  bool ready() const;
  RenderReceipt diagnostics(std::string_view result, std::string_view reasonCode) const;

private:
  CommandRecordingCreateInfo createInfo_;
  VkCommandPool commandPool_{};
  std::vector<VkCommandBuffer> commandBuffers_;
  bool ready_ = false;
};

}  // namespace iggy3d::vulkan
