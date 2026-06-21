#include "render/vulkan/FrameSync.hpp"

#include <algorithm>

#include "render/vulkan/VulkanResult.hpp"

namespace iggy3d::vulkan {
namespace {

RenderReason reasonFor(std::string_view code) {
  if (code == "vulkan_smoke_pass") {
    return {code, "vulkan smoke pass"};
  }
  if (code == "semaphore_create_failed") {
    return {code, "semaphore create failed"};
  }
  if (code == "fence_create_failed") {
    return {code, "fence create failed"};
  }
  if (code == "fence_wait_timeout") {
    return {code, "fence wait timeout"};
  }
  if (code == "fence_wait_failed") {
    return {code, "fence wait failed"};
  }
  if (code == "fence_reset_failed") {
    return {code, "fence reset failed"};
  }
  if (code == "sync_validation_failed") {
    return {code, "sync validation failed"};
  }
  return {"sync_not_ready", "frame sync not ready"};
}

std::string_view fenceStateName(const FrameSlotSync& slot) {
  if (slot.submitInFlight) {
    return "in_flight";
  }
  return slot.fenceKnownSignaled ? "signaled" : "unknown";
}

}  // namespace

FrameSync::~FrameSync() {
  destroy();
}

RenderReceipt FrameSync::diagnostics(std::string_view result,
                                     std::string_view reasonCode) const {
  RenderReceipt receipt;
  appendReceiptField(receipt, "receipt_version", "1");
  appendReceiptField(receipt, "repo", "iggy3d");
  appendReceiptField(receipt, "file_plan", "src/render/vulkan/FrameSync.cpp");
  appendReceiptField(receipt, "packet_order", "5");
  appendReceiptField(receipt, "backend", "vulkan");
  appendReceiptField(receipt, "sync_policy", "binary_wsi");
  appendReceiptField(receipt, "frame_slots", static_cast<std::uint64_t>(slots_.size()));
  appendReceiptField(receipt, "current_frame_slot",
                     static_cast<std::uint64_t>(currentFrameSlot_));
  if (!slots_.empty()) {
    appendReceiptField(receipt, "frame_fence_state", fenceStateName(slots_[currentFrameSlot_]));
  } else {
    appendReceiptField(receipt, "frame_fence_state", "unavailable");
  }
  appendReceiptField(receipt, "fence_wait_timeout_ns", createInfo_.fenceWaitTimeoutNs);
  appendReceiptField(receipt, "fence_wait_retry_count",
                     static_cast<std::uint64_t>(createInfo_.fenceWaitRetryCount));
  appendReceiptField(receipt, "result", result);
  appendReceiptField(receipt, "reason_code", reasonCode);
  return receipt;
}

FrameSyncOperationResult FrameSync::create(const FrameSyncCreateInfo& createInfo) {
  destroy();
  createInfo_ = createInfo;
  createInfo_.frameSlotCount = std::clamp(createInfo_.frameSlotCount, 1U, 3U);
  slots_.resize(createInfo_.frameSlotCount);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (FrameSlotSync& slot : slots_) {
    if (vkCreateSemaphore(createInfo_.device, &semaphoreInfo, nullptr,
                          &slot.imageAvailable) != VK_SUCCESS ||
        vkCreateSemaphore(createInfo_.device, &semaphoreInfo, nullptr,
                          &slot.renderFinished) != VK_SUCCESS) {
      FrameSyncOperationResult result;
      result.outcome = RenderOutcome::FatalRendererError;
      result.reason = reasonFor("semaphore_create_failed");
      destroy();
      result.receipt = diagnostics("fail", result.reason.code);
      return result;
    }
    if (vkCreateFence(createInfo_.device, &fenceInfo, nullptr, &slot.inFlightFence) !=
        VK_SUCCESS) {
      FrameSyncOperationResult result;
      result.outcome = RenderOutcome::FatalRendererError;
      result.reason = reasonFor("fence_create_failed");
      destroy();
      result.receipt = diagnostics("fail", result.reason.code);
      return result;
    }
    slot.fenceKnownSignaled = true;
  }

  ready_ = true;
  FrameSyncOperationResult result;
  result.outcome = RenderOutcome::Ok;
  result.reason = reasonFor("vulkan_smoke_pass");
  result.receipt = diagnostics("pass", result.reason.code);
  appendReceiptField(result.receipt, "image_available_semaphore_count",
                     static_cast<std::uint64_t>(slots_.size()));
  appendReceiptField(result.receipt, "render_finished_semaphore_count",
                     static_cast<std::uint64_t>(slots_.size()));
  appendReceiptField(result.receipt, "in_flight_fence_count",
                     static_cast<std::uint64_t>(slots_.size()));
  return result;
}

void FrameSync::destroy() {
  if (createInfo_.device != VK_NULL_HANDLE) {
    for (FrameSlotSync& slot : slots_) {
      if (slot.inFlightFence != VK_NULL_HANDLE) {
        vkDestroyFence(createInfo_.device, slot.inFlightFence, nullptr);
      }
      if (slot.renderFinished != VK_NULL_HANDLE) {
        vkDestroySemaphore(createInfo_.device, slot.renderFinished, nullptr);
      }
      if (slot.imageAvailable != VK_NULL_HANDLE) {
        vkDestroySemaphore(createInfo_.device, slot.imageAvailable, nullptr);
      }
    }
  }
  slots_.clear();
  currentFrameSlot_ = 0;
  ready_ = false;
}

std::uint32_t FrameSync::currentFrameSlot() const {
  return currentFrameSlot_;
}

const FrameSlotSync& FrameSync::currentSlot() const {
  return slots_[currentFrameSlot_];
}

FrameSlotSync& FrameSync::currentSlot() {
  return slots_[currentFrameSlot_];
}

FrameSyncWaitResult FrameSync::waitForCurrentFrame() {
  FrameSyncWaitResult result;
  if (!ready_) {
    result.outcome = RenderOutcome::RendererNotReady;
    result.reason = reasonFor("sync_not_ready");
    result.receipt = diagnostics("fail", result.reason.code);
    appendReceiptField(result.receipt, "fence_wait_result", "not_ready");
    return result;
  }
  FrameSlotSync& slot = currentSlot();
  for (std::uint32_t attempt = 0; attempt < createInfo_.fenceWaitRetryCount; ++attempt) {
    const VkResult waitResult = vkWaitForFences(createInfo_.device, 1, &slot.inFlightFence,
                                                VK_TRUE, createInfo_.fenceWaitTimeoutNs);
    if (waitResult == VK_SUCCESS) {
      slot.fenceKnownSignaled = true;
      slot.submitInFlight = false;
      result.outcome = RenderOutcome::Ok;
      result.reason = reasonFor("vulkan_smoke_pass");
      result.fenceReady = true;
      result.receipt = diagnostics("pass", result.reason.code);
      appendReceiptField(result.receipt, "fence_wait_result", vkResultName(waitResult));
      return result;
    }
    if (waitResult != VK_TIMEOUT) {
      result.outcome = mapVkResult(waitResult, VulkanCallContext::FenceWait).outcome;
      result.reason = reasonFor("fence_wait_failed");
      result.receipt = diagnostics("fail", result.reason.code);
      appendReceiptField(result.receipt, "fence_wait_result", vkResultName(waitResult));
      return result;
    }
  }
  result.outcome = RenderOutcome::SkipFrame;
  result.reason = reasonFor("fence_wait_timeout");
  result.receipt = diagnostics("skip", result.reason.code);
  appendReceiptField(result.receipt, "fence_wait_result", "VK_TIMEOUT");
  return result;
}

FrameSyncOperationResult FrameSync::resetFenceBeforeSubmit() {
  FrameSyncOperationResult result;
  if (!ready_) {
    result.outcome = RenderOutcome::RendererNotReady;
    result.reason = reasonFor("sync_not_ready");
    result.receipt = diagnostics("fail", result.reason.code);
    return result;
  }
  FrameSlotSync& slot = currentSlot();
  const VkResult resetResult = vkResetFences(createInfo_.device, 1, &slot.inFlightFence);
  if (resetResult != VK_SUCCESS) {
    result.outcome = mapVkResult(resetResult, VulkanCallContext::FenceWait).outcome;
    result.reason = reasonFor("fence_reset_failed");
    result.receipt = diagnostics("fail", result.reason.code);
    appendReceiptField(result.receipt, "fence_reset_result", vkResultName(resetResult));
    return result;
  }
  slot.fenceKnownSignaled = false;
  result.outcome = RenderOutcome::Ok;
  result.reason = reasonFor("vulkan_smoke_pass");
  result.receipt = diagnostics("pass", result.reason.code);
  appendReceiptField(result.receipt, "fence_reset_result", vkResultName(resetResult));
  return result;
}

FrameSyncSubmitPlan FrameSync::submitPlanForCurrentFrame() const {
  const FrameSlotSync& slot = currentSlot();
  FrameSyncSubmitPlan plan;
  plan.waitSemaphore = slot.imageAvailable;
  plan.signalSemaphore = slot.renderFinished;
  plan.signalFence = slot.inFlightFence;
  return plan;
}

void FrameSync::markSubmitted(std::uint32_t imageIndex) {
  FrameSlotSync& slot = currentSlot();
  slot.submitInFlight = true;
  slot.fenceKnownSignaled = false;
  slot.lastAcquiredImageIndex = imageIndex;
}

void FrameSync::markPresentedOrSkipped(bool submitted) {
  FrameSlotSync& slot = currentSlot();
  if (!submitted) {
    slot.submitInFlight = false;
  }
}

void FrameSync::advanceFrameSlot() {
  if (!slots_.empty()) {
    currentFrameSlot_ = (currentFrameSlot_ + 1U) % static_cast<std::uint32_t>(slots_.size());
  }
}

bool FrameSync::ready() const {
  return ready_;
}

}  // namespace iggy3d::vulkan
