#pragma once

#include <cstdint>
#include <vector>

#include "render/RenderDiagnostics.hpp"
#include "render/vulkan/VulkanFunctions.hpp"
#include "render/vulkan/VulkanTypes.hpp"

namespace iggy3d::vulkan {

struct CommandRecordingCreateInfo {
  VkDevice device{};
  VulkanDeviceFunctions deviceFunctions;
  std::uint32_t graphicsQueueFamily = kInvalidVulkanQueueFamily;
  std::uint32_t frameSlotCount = 2;
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
  bool ready() const;
  RenderReceipt diagnostics(std::string_view result, std::string_view reasonCode) const;

private:
  CommandRecordingCreateInfo createInfo_;
  VkCommandPool commandPool_{};
  std::vector<VkCommandBuffer> commandBuffers_;
  bool ready_ = false;
};

}  // namespace iggy3d::vulkan
