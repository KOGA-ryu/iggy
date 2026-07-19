#include "render/vulkan/RenderLoopSubmission.hpp"

namespace iggy3d::vulkan {

RenderLoopSubmissionResult submitAndPresentRenderLoopFrame(
    const RenderLoopCreateInfo& createInfo,
    const FrameSyncSubmitPlan& submitPlan, VkCommandBuffer commandBuffer,
    std::uint32_t imageIndex) {
  RenderLoopSubmissionResult result;
  result.reset = createInfo.frameSync->resetFenceBeforeSubmit();
  if (result.reset.outcome != RenderOutcome::Ok) {
    return result;
  }

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
  result.submitResult =
      vkQueueSubmit(createInfo.deviceSurface->handles().graphicsQueue, 1,
                    &submitInfo, submitPlan.signalFence);
  if (result.submitResult != VK_SUCCESS) {
    result.stage = RenderLoopSubmissionStage::SubmitFailed;
    return result;
  }
  createInfo.frameSync->markSubmitted(imageIndex);

  VkSwapchainKHR swapchain = createInfo.swapchain->handle();
  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &signalSemaphore;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &swapchain;
  presentInfo.pImageIndices = &imageIndex;
  result.presentResult =
      createInfo.deviceSurface->functions().device.queuePresentKHR(
          createInfo.deviceSurface->handles().presentQueue, &presentInfo);
  result.presentation =
      createInfo.swapchain->notePresentResult(result.presentResult);
  createInfo.frameSync->markPresentedOrSkipped(true);
  createInfo.frameSync->advanceFrameSlot();
  result.stage = RenderLoopSubmissionStage::PresentComplete;
  return result;
}

}  // namespace iggy3d::vulkan
