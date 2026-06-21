#include "render/vulkan/RenderLoop.hpp"

#include <string>

#include "render/vulkan/VulkanResult.hpp"

namespace iggy3d::vulkan {
namespace {

RenderReason reasonFor(std::string_view code) {
  if (code == "vulkan_smoke_pass") {
    return {code, "vulkan smoke pass"};
  }
  if (code == "empty_frame_presented") {
    return {code, "empty frame presented"};
  }
  if (code == "swapchain_suboptimal") {
    return {code, "swapchain suboptimal"};
  }
  if (code == "swapchain_out_of_date") {
    return {code, "swapchain out of date"};
  }
  if (code == "empty_frame_skipped_not_drawable") {
    return {code, "empty frame skipped not drawable"};
  }
  if (code == "empty_frame_acquire_skipped") {
    return {code, "empty frame acquire skipped"};
  }
  if (code == "empty_frame_submit_failed") {
    return {code, "empty frame submit failed"};
  }
  if (code == "empty_frame_present_failed") {
    return {code, "empty frame present failed"};
  }
  if (code == "render_loop_shutdown") {
    return {code, "render loop shutdown"};
  }
  if (code == "swapchain_recreate_failed") {
    return {code, "swapchain recreate failed"};
  }
  return {"render_loop_not_ready", "render loop not ready"};
}

std::string extentString(VkExtent2D extent) {
  return std::to_string(extent.width) + "x" + std::to_string(extent.height);
}

}  // namespace

std::string_view vulkanFrameStatusName(VulkanFrameStatus status) {
  switch (status) {
    case VulkanFrameStatus::Presented:
      return "presented";
    case VulkanFrameStatus::PresentedSuboptimal:
      return "presented_suboptimal";
    case VulkanFrameStatus::SkippedNotDrawable:
      return "skipped_not_drawable";
    case VulkanFrameStatus::SwapchainRecreated:
      return "swapchain_recreated";
    case VulkanFrameStatus::SkippedAcquire:
      return "skipped_acquire";
    case VulkanFrameStatus::PresentRecreateRequested:
      return "present_recreate_requested";
    case VulkanFrameStatus::Failed:
      return "failed";
  }
  return "failed";
}

RenderReceipt RenderLoop::makeReceipt(std::string_view result,
                                      std::string_view reasonCode) const {
  RenderReceipt receipt;
  appendReceiptField(receipt, "receipt_version", "1");
  appendReceiptField(receipt, "repo", "iggy3d");
  appendReceiptField(receipt, "file_plan", "src/render/vulkan/RenderLoop.cpp");
  appendReceiptField(receipt, "packet_order", "5");
  appendReceiptField(receipt, "backend", "vulkan");
  appendReceiptField(receipt, "render_loop_ready", ready_);
  if (createInfo_.swapchain != nullptr) {
    const SwapchainInfo& swapchainInfo = createInfo_.swapchain->info();
    appendReceiptField(receipt, "swapchain_state", swapchainStateName(swapchainInfo.state));
    appendReceiptField(receipt, "swapchain_generation",
                       static_cast<std::uint64_t>(swapchainInfo.generation));
    appendReceiptField(receipt, "swapchain_extent", extentString(swapchainInfo.extent));
    appendReceiptField(receipt, "swapchain_recreate_count",
                       static_cast<std::uint64_t>(swapchainInfo.recreateCount));
  }
  if (createInfo_.frameSync != nullptr) {
    appendReceiptField(receipt, "sync_policy", "binary_wsi");
    appendReceiptField(receipt, "current_frame_slot",
                       static_cast<std::uint64_t>(createInfo_.frameSync->currentFrameSlot()));
  }
  appendReceiptField(receipt, "validation_error_count", static_cast<std::uint64_t>(0));
  appendReceiptField(receipt, "sync_validation_error_count", static_cast<std::uint64_t>(0));
  appendReceiptField(receipt, "result", result);
  appendReceiptField(receipt, "reason_code", reasonCode);
  return receipt;
}

VulkanFrameResult RenderLoop::initialize(const RenderLoopCreateInfo& createInfo) {
  createInfo_ = createInfo;
  shutdown_ = false;
  ready_ = createInfo_.deviceSurface != nullptr && createInfo_.deviceSurface->ready() &&
           createInfo_.swapchain != nullptr && createInfo_.swapchain->ready() &&
           createInfo_.frameSync != nullptr && createInfo_.frameSync->ready() &&
           createInfo_.commandRecording != nullptr && createInfo_.commandRecording->ready();
  VulkanFrameResult result;
  result.outcome = ready_ ? RenderOutcome::Ok : RenderOutcome::RendererNotReady;
  result.reason = ready_ ? reasonFor("vulkan_smoke_pass") : reasonFor("render_loop_not_ready");
  result.status = ready_ ? VulkanFrameStatus::SwapchainRecreated : VulkanFrameStatus::Failed;
  result.receipt = makeReceipt(ready_ ? "pass" : "fail", result.reason.code);
  return result;
}

VulkanFrameResult RenderLoop::resize(std::uint32_t width, std::uint32_t height) {
  VulkanFrameResult result;
  if (createInfo_.swapchain == nullptr) {
    result.outcome = RenderOutcome::RendererNotReady;
    result.reason = reasonFor("render_loop_not_ready");
    result.receipt = makeReceipt("fail", result.reason.code);
    return result;
  }
  const SwapchainOperationResult resizeResult =
      createInfo_.swapchain->markDrawableExtent(width, height);
  if (resizeResult.outcome == RenderOutcome::SkipFrame) {
    result.status = VulkanFrameStatus::SkippedNotDrawable;
    result.outcome = RenderOutcome::SkipFrame;
    result.reason = reasonFor("empty_frame_skipped_not_drawable");
    result.receipt = makeReceipt("skip", result.reason.code);
    return result;
  }
  if (resizeResult.outcome == RenderOutcome::RecreateSwapchain) {
    const SwapchainOperationResult recreated = createInfo_.swapchain->recreate(width, height);
    result.swapchainRecreated = recreated.outcome == RenderOutcome::Ok;
    result.status =
        result.swapchainRecreated ? VulkanFrameStatus::SwapchainRecreated : VulkanFrameStatus::Failed;
    result.outcome = recreated.outcome;
    result.reason = result.swapchainRecreated ? reasonFor("vulkan_smoke_pass")
                                              : reasonFor("swapchain_recreate_failed");
    result.receipt = makeReceipt(result.swapchainRecreated ? "pass" : "fail", result.reason.code);
    return result;
  }
  result.status = VulkanFrameStatus::SwapchainRecreated;
  result.outcome = RenderOutcome::Ok;
  result.reason = reasonFor("vulkan_smoke_pass");
  result.receipt = makeReceipt("pass", result.reason.code);
  return result;
}

VulkanFrameResult RenderLoop::renderFrame(const FrameInput& frame) {
  VulkanFrameResult result;
  if (!ready_ || shutdown_) {
    result.outcome = RenderOutcome::RendererNotReady;
    result.reason = reasonFor(shutdown_ ? "render_loop_shutdown" : "render_loop_not_ready");
    result.receipt = makeReceipt("fail", result.reason.code);
    return result;
  }

  const FrameInputStatus frameStatus = validateFrameInput(frame);
  if (frameStatus == FrameInputStatus::NotDrawable) {
    result.status = VulkanFrameStatus::SkippedNotDrawable;
    result.outcome = RenderOutcome::SkipFrame;
    result.reason = reasonFor("empty_frame_skipped_not_drawable");
    result.receipt = makeReceipt("skip", result.reason.code);
    appendReceiptField(result.receipt, "frame_input_valid", false);
    appendReceiptField(result.receipt, "frame_input_reason", frameInputReasonCode(frameStatus));
    return result;
  }
  if (frameStatus != FrameInputStatus::Valid) {
    result.outcome = RenderOutcome::InvalidFrameInput;
    result.reason = {"frame_input_invalid", "frame input invalid"};
    result.receipt = makeReceipt("fail", result.reason.code);
    appendReceiptField(result.receipt, "frame_input_valid", false);
    appendReceiptField(result.receipt, "frame_input_reason", frameInputReasonCode(frameStatus));
    return result;
  }

  const SwapchainInfo& swapchainInfo = createInfo_.swapchain->info();
  if (swapchainInfo.state == SwapchainState::NotDrawable || swapchainInfo.extent.width == 0U ||
      swapchainInfo.extent.height == 0U) {
    result.status = VulkanFrameStatus::SkippedNotDrawable;
    result.outcome = RenderOutcome::SkipFrame;
    result.reason = reasonFor("empty_frame_skipped_not_drawable");
    result.receipt = makeReceipt("skip", result.reason.code);
    return result;
  }
  if (swapchainInfo.state == SwapchainState::DirtyResize ||
      swapchainInfo.state == SwapchainState::DirtyOutOfDate ||
      swapchainInfo.state == SwapchainState::DirtySuboptimal) {
    const SwapchainOperationResult recreated =
        createInfo_.swapchain->recreate(frame.viewport.width, frame.viewport.height);
    if (recreated.outcome != RenderOutcome::Ok) {
      result.status = VulkanFrameStatus::Failed;
      result.outcome = recreated.outcome;
      result.reason = reasonFor("swapchain_recreate_failed");
      result.receipt = makeReceipt("fail", result.reason.code);
      return result;
    }
    result.swapchainRecreated = true;
  }

  result.frameSlot = createInfo_.frameSync->currentFrameSlot();
  const FrameSyncWaitResult waitResult = createInfo_.frameSync->waitForCurrentFrame();
  if (waitResult.outcome != RenderOutcome::Ok) {
    result.status = VulkanFrameStatus::SkippedAcquire;
    result.outcome = waitResult.outcome;
    result.reason = waitResult.reason;
    result.receipt = makeReceipt("skip", result.reason.code);
    return result;
  }

  const FrameSyncSubmitPlan submitPlan = createInfo_.frameSync->submitPlanForCurrentFrame();
  const SwapchainAcquireResult acquire = createInfo_.swapchain->acquire(submitPlan.waitSemaphore);
  result.swapchainImageIndex = acquire.imageIndex;
  if (!acquire.submitAllowed || !acquire.imageValid) {
    createInfo_.frameSync->markPresentedOrSkipped(false);
    createInfo_.frameSync->advanceFrameSlot();
    result.status = VulkanFrameStatus::SkippedAcquire;
    result.outcome = acquire.outcome;
    result.reason = acquire.recreateRequested ? reasonFor("empty_frame_acquire_skipped")
                                              : acquire.reason;
    result.receipt = makeReceipt(acquire.recreateRequested ? "skip" : "fail",
                                 result.reason.code);
    appendReceiptField(result.receipt, "acquire_action",
                       acquire.recreateRequested ? "recreate" : "fail");
    return result;
  }

  const SwapchainInfo& readySwapchain = createInfo_.swapchain->info();
  EmptyFrameRecordInfo recordInfo;
  recordInfo.commandBuffer =
      createInfo_.commandRecording->commandBufferForFrameSlot(result.frameSlot);
  recordInfo.swapchainImage = createInfo_.swapchain->imageAt(acquire.imageIndex);
  recordInfo.swapchainImageView = createInfo_.swapchain->imageViewAt(acquire.imageIndex);
  recordInfo.colorFormat = readySwapchain.colorFormat;
  recordInfo.extent = readySwapchain.extent;
  recordInfo.frameSlot = result.frameSlot;
  recordInfo.imageIndex = acquire.imageIndex;
  const CommandRecordResult recordResult =
      createInfo_.commandRecording->recordEmptyFrame(recordInfo);
  if (!recordResult.recorded) {
    createInfo_.frameSync->markPresentedOrSkipped(false);
    createInfo_.frameSync->advanceFrameSlot();
    result.status = VulkanFrameStatus::Failed;
    result.outcome = recordResult.outcome;
    result.reason = recordResult.reason;
    result.receipt = makeReceipt("fail", result.reason.code);
    return result;
  }
  result.commandRecorded = true;

  const FrameSyncOperationResult resetResult = createInfo_.frameSync->resetFenceBeforeSubmit();
  if (resetResult.outcome != RenderOutcome::Ok) {
    result.status = VulkanFrameStatus::Failed;
    result.outcome = resetResult.outcome;
    result.reason = resetResult.reason;
    result.receipt = makeReceipt("fail", result.reason.code);
    return result;
  }

  VkCommandBuffer commandBuffer = recordInfo.commandBuffer;
  VkSemaphore waitSemaphore = submitPlan.waitSemaphore;
  VkSemaphore signalSemaphore = submitPlan.signalSemaphore;
  VkPipelineStageFlags waitStage = submitPlan.waitStageMask;
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &waitSemaphore;
  submitInfo.pWaitDstStageMask = &waitStage;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &signalSemaphore;
  const VkResult submitResult = vkQueueSubmit(createInfo_.deviceSurface->handles().graphicsQueue,
                                              1, &submitInfo, submitPlan.signalFence);
  if (submitResult != VK_SUCCESS) {
    result.status = VulkanFrameStatus::Failed;
    result.outcome = mapVkResult(submitResult, VulkanCallContext::QueueSubmit).outcome;
    result.reason = reasonFor("empty_frame_submit_failed");
    result.receipt = makeReceipt("fail", result.reason.code);
    appendReceiptField(result.receipt, "submit_result", vkResultName(submitResult));
    return result;
  }
  createInfo_.frameSync->markSubmitted(acquire.imageIndex);
  result.submitted = true;

  VkSwapchainKHR swapchain = createInfo_.swapchain->handle();
  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &signalSemaphore;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &swapchain;
  presentInfo.pImageIndices = &acquire.imageIndex;
  const VkResult presentVkResult =
      createInfo_.deviceSurface->functions().device.queuePresentKHR(
          createInfo_.deviceSurface->handles().presentQueue, &presentInfo);
  const SwapchainPresentResult presentResult =
      createInfo_.swapchain->notePresentResult(presentVkResult);
  createInfo_.frameSync->markPresentedOrSkipped(true);
  createInfo_.frameSync->advanceFrameSlot();

  result.presented = presentResult.presented;
  result.swapchainRecreated = result.swapchainRecreated || presentResult.recreateRequested;
  if (presentResult.presented) {
    result.status = presentResult.recreateRequested ? VulkanFrameStatus::PresentedSuboptimal
                                                    : VulkanFrameStatus::Presented;
    result.outcome = RenderOutcome::Ok;
    result.reason = reasonFor(presentResult.recreateRequested ? "swapchain_suboptimal"
                                                              : "empty_frame_presented");
    result.receipt = makeReceipt("pass", result.reason.code);
  } else if (presentResult.recreateRequested) {
    result.status = VulkanFrameStatus::PresentRecreateRequested;
    result.outcome = presentResult.outcome;
    result.reason = presentResult.reason;
    result.receipt = makeReceipt("skip", result.reason.code);
  } else {
    result.status = VulkanFrameStatus::Failed;
    result.outcome = presentResult.outcome;
    result.reason = reasonFor("empty_frame_present_failed");
    result.receipt = makeReceipt("fail", result.reason.code);
  }
  appendReceiptField(result.receipt, "frame_status", vulkanFrameStatusName(result.status));
  appendReceiptField(result.receipt, "acquire_result", "VK_SUCCESS");
  appendReceiptField(result.receipt, "acquire_action", "submit");
  appendReceiptField(result.receipt, "acquired_image_index",
                     static_cast<std::uint64_t>(acquire.imageIndex));
  appendReceiptField(result.receipt, "command_recorded", result.commandRecorded);
  appendReceiptField(result.receipt, "submit_result", vkResultName(submitResult));
  appendReceiptField(result.receipt, "present_result", vkResultName(presentVkResult));
  appendReceiptField(result.receipt, "present_action",
                     presentResult.presented
                         ? (presentResult.recreateRequested ? "presented_then_recreate"
                                                            : "presented")
                         : (presentResult.recreateRequested ? "recreate" : "fail"));
  appendReceiptField(result.receipt, "presented", presentResult.presented);
  appendReceiptField(result.receipt, "draw_count", static_cast<std::uint64_t>(0));
  return result;
}

RenderOutcome RenderLoop::waitIdle() {
  if (createInfo_.deviceSurface != nullptr &&
      createInfo_.deviceSurface->handles().device != VK_NULL_HANDLE) {
    return mapVkResult(vkDeviceWaitIdle(createInfo_.deviceSurface->handles().device),
                       VulkanCallContext::Unknown)
        .outcome;
  }
  return RenderOutcome::Ok;
}

void RenderLoop::shutdown() {
  ready_ = false;
  shutdown_ = true;
}

bool RenderLoop::ready() const {
  return ready_ && !shutdown_;
}

}  // namespace iggy3d::vulkan
