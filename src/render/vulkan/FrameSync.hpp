#pragma once

#include <cstdint>
#include <vector>

#include "render/RenderDiagnostics.hpp"
#include "render/vulkan/VulkanTypes.hpp"

namespace iggy3d::vulkan {

struct FrameSyncCreateInfo {
  VkDevice device{};
  std::uint32_t frameSlotCount = 2;
  std::uint64_t fenceWaitTimeoutNs = 1'000'000'000ULL;
  std::uint32_t fenceWaitRetryCount = 5;
};

struct FrameSlotSync {
  VkSemaphore imageAvailable{};
  VkSemaphore renderFinished{};
  VkFence inFlightFence{};
  bool fenceKnownSignaled = false;
  bool submitInFlight = false;
  std::uint32_t lastAcquiredImageIndex = 0;
};

struct FrameSyncOperationResult {
  RenderOutcome outcome = RenderOutcome::RendererNotReady;
  RenderReason reason{"sync_not_ready", "frame sync not ready"};
  RenderReceipt receipt;
};

struct FrameSyncWaitResult {
  RenderOutcome outcome = RenderOutcome::RendererNotReady;
  RenderReason reason{"sync_not_ready", "frame sync not ready"};
  bool fenceReady = false;
  RenderReceipt receipt;
};

struct FrameSyncSubmitPlan {
  VkSemaphore waitSemaphore{};
  VkSemaphore signalSemaphore{};
  VkFence signalFence{};
  VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
};

class FrameSync {
public:
  FrameSync() = default;
  ~FrameSync();

  FrameSync(const FrameSync&) = delete;
  FrameSync& operator=(const FrameSync&) = delete;

  FrameSyncOperationResult create(const FrameSyncCreateInfo& createInfo);
  void destroy();

  std::uint32_t currentFrameSlot() const;
  const FrameSlotSync& currentSlot() const;
  FrameSlotSync& currentSlot();
  FrameSyncWaitResult waitForCurrentFrame();
  FrameSyncOperationResult resetFenceBeforeSubmit();
  FrameSyncSubmitPlan submitPlanForCurrentFrame() const;
  void markSubmitted(std::uint32_t imageIndex);
  void markPresentedOrSkipped(bool submitted);
  void advanceFrameSlot();
  bool ready() const;
  RenderReceipt diagnostics(std::string_view result, std::string_view reasonCode) const;

private:
  FrameSyncCreateInfo createInfo_;
  std::vector<FrameSlotSync> slots_;
  std::uint32_t currentFrameSlot_ = 0;
  bool ready_ = false;
};

}  // namespace iggy3d::vulkan
