#pragma once

#include <cstdint>

#include "render/FrameInput.hpp"
#include "render/vulkan/CommandRecording.hpp"
#include "render/vulkan/FrameSync.hpp"
#include "render/vulkan/InstanceDeviceSurface.hpp"
#include "render/vulkan/BufferImageResources.hpp"
#include "render/vulkan/FirstRoomPipeline.hpp"
#include "render/vulkan/FrameCapture.hpp"
#include "render/vulkan/PipelineLayout.hpp"
#include "render/vulkan/Swapchain.hpp"

namespace iggy3d::vulkan {

enum class VulkanFrameStatus : std::uint8_t {
  Presented,
  PresentedSuboptimal,
  SkippedNotDrawable,
  SwapchainRecreated,
  SkippedAcquire,
  PresentRecreateRequested,
  Failed,
};

struct VulkanFrameResult {
  VulkanFrameStatus status = VulkanFrameStatus::Failed;
  RenderOutcome outcome = RenderOutcome::RendererNotReady;
  RenderReason reason{"render_loop_not_ready", "render loop not ready"};
  bool presented = false;
  bool swapchainRecreated = false;
  bool commandRecorded = false;
  bool submitted = false;
  std::uint32_t frameSlot = 0;
  std::uint32_t swapchainImageIndex = 0;
  RenderReceipt receipt;
};

struct RenderLoopCreateInfo {
  InstanceDeviceSurface* deviceSurface = nullptr;
  Swapchain* swapchain = nullptr;
  FrameSync* frameSync = nullptr;
  CommandRecording* commandRecording = nullptr;
  FirstRoomPipelineRecord* firstRoomPipeline = nullptr;
  PipelineLayoutRecord* firstRoomLayout = nullptr;
  BufferImageResources* firstRoomResources = nullptr;
  FrameCapture* frameCapture = nullptr;
};

class RenderLoop {
public:
  RenderLoop() = default;

  RenderLoop(const RenderLoop&) = delete;
  RenderLoop& operator=(const RenderLoop&) = delete;

  VulkanFrameResult initialize(const RenderLoopCreateInfo& createInfo);
  VulkanFrameResult renderFrame(const FrameInput& frame);
  VulkanFrameResult resize(std::uint32_t width, std::uint32_t height);
  RenderOutcome waitIdle();
  void shutdown();
  bool ready() const;

private:
  RenderReceipt makeReceipt(std::string_view result, std::string_view reasonCode) const;

  RenderLoopCreateInfo createInfo_;
  bool ready_ = false;
  bool shutdown_ = false;
};

std::string_view vulkanFrameStatusName(VulkanFrameStatus status);

}  // namespace iggy3d::vulkan
