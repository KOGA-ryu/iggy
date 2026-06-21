#include "render/vulkan/VulkanResult.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool maps(VkResult raw,
          iggy3d::vulkan::VulkanCallContext context,
          iggy3d::RenderOutcome outcome,
          std::string_view reason) {
  const iggy3d::vulkan::VulkanResultMapping mapping =
      iggy3d::vulkan::mapVkResult(raw, context);
  return expect(mapping.outcome == outcome, "outcome mismatch") &&
         expect(mapping.reason.code == reason, "reason mismatch") &&
         expect(mapping.rawResultName != std::string_view{"VK_RESULT_UNMAPPED"},
                "raw result named");
}

bool explicitMappingsAreStable() {
  bool ok = true;
  ok = maps(VK_SUCCESS, iggy3d::vulkan::VulkanCallContext::InstanceCreate,
            iggy3d::RenderOutcome::Ok, "vk_success") &&
       ok;
  ok = maps(VK_NOT_READY, iggy3d::vulkan::VulkanCallContext::AcquireImage,
            iggy3d::RenderOutcome::SkipFrame, "vk_not_ready") &&
       ok;
  ok = maps(VK_TIMEOUT, iggy3d::vulkan::VulkanCallContext::FenceWait,
            iggy3d::RenderOutcome::SkipFrame, "vk_timeout") &&
       ok;
  ok = maps(VK_ERROR_OUT_OF_DATE_KHR, iggy3d::vulkan::VulkanCallContext::Present,
            iggy3d::RenderOutcome::RecreateSwapchain, "swapchain_out_of_date") &&
       ok;
  ok = maps(VK_SUBOPTIMAL_KHR, iggy3d::vulkan::VulkanCallContext::Present,
            iggy3d::RenderOutcome::SkipFrame, "swapchain_suboptimal") &&
       ok;
  ok = maps(VK_ERROR_EXTENSION_NOT_PRESENT,
            iggy3d::vulkan::VulkanCallContext::InstanceCreate,
            iggy3d::RenderOutcome::Unsupported, "unsupported_required_extension") &&
       ok;
  ok = maps(VK_ERROR_FEATURE_NOT_PRESENT, iggy3d::vulkan::VulkanCallContext::DeviceCreate,
            iggy3d::RenderOutcome::Unsupported, "unsupported_required_feature") &&
       ok;
  ok = maps(VK_ERROR_LAYER_NOT_PRESENT, iggy3d::vulkan::VulkanCallContext::InstanceCreate,
            iggy3d::RenderOutcome::Unsupported, "validation_layer_missing") &&
       ok;
  ok = maps(VK_ERROR_INCOMPATIBLE_DRIVER,
            iggy3d::vulkan::VulkanCallContext::InstanceCreate,
            iggy3d::RenderOutcome::Unsupported, "incompatible_driver") &&
       ok;
  ok = maps(VK_ERROR_VALIDATION_FAILED_EXT,
            iggy3d::vulkan::VulkanCallContext::QueueSubmit,
            iggy3d::RenderOutcome::ValidationFailure, "validation_failed") &&
       ok;
  return ok;
}

bool deviceLostIsOnlyDeviceLost() {
  const iggy3d::vulkan::VulkanResultMapping mapping =
      iggy3d::vulkan::mapVkResult(VK_ERROR_DEVICE_LOST,
                                  iggy3d::vulkan::VulkanCallContext::Present);
  return expect(mapping.outcome == iggy3d::RenderOutcome::DeviceLost, "device lost outcome") &&
         expect(mapping.reason.code == "device_lost", "device lost reason") &&
         expect(mapping.deviceLost, "device lost flag") &&
         expect(!mapping.surfaceInvalid, "device lost not surface invalid");
}

bool unknownIsDeterministic() {
  const iggy3d::vulkan::VulkanResultMapping mapping =
      iggy3d::vulkan::mapVkResult(static_cast<VkResult>(-999999),
                                  iggy3d::vulkan::VulkanCallContext::Unknown);
  return expect(mapping.outcome == iggy3d::RenderOutcome::FatalRendererError,
                "unknown outcome") &&
         expect(mapping.reason.code == "vulkan_result_unmapped", "unknown reason") &&
         expect(mapping.rawResultName == std::string_view{"VK_RESULT_UNMAPPED"},
                "unknown raw name");
}

}  // namespace

int main() {
  bool ok = true;
  ok = explicitMappingsAreStable() && ok;
  ok = deviceLostIsOnlyDeviceLost() && ok;
  ok = unknownIsDeterministic() && ok;
  return ok ? 0 : 1;
}
