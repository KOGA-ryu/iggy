#include "render/vulkan/VulkanResult.hpp"

namespace iggy3d::vulkan {
namespace {

RenderReason reason(std::string_view code) {
  if (code == "vk_success") {
    return {code, "vulkan success"};
  }
  if (code == "vk_not_ready") {
    return {code, "vulkan not ready"};
  }
  if (code == "vk_timeout") {
    return {code, "vulkan timeout"};
  }
  if (code == "swapchain_out_of_date") {
    return {code, "swapchain out of date"};
  }
  if (code == "swapchain_suboptimal") {
    return {code, "swapchain suboptimal"};
  }
  if (code == "device_lost") {
    return {code, "device lost"};
  }
  if (code == "unsupported_required_extension") {
    return {code, "unsupported required extension"};
  }
  if (code == "unsupported_required_feature") {
    return {code, "unsupported required feature"};
  }
  if (code == "validation_layer_missing") {
    return {code, "validation layer missing"};
  }
  if (code == "incompatible_driver") {
    return {code, "incompatible driver"};
  }
  if (code == "format_not_supported") {
    return {code, "format not supported"};
  }
  if (code == "native_window_in_use") {
    return {code, "native window in use"};
  }
  if (code == "validation_failed") {
    return {code, "validation failed"};
  }
  return {"vulkan_result_unmapped", "vulkan result unmapped"};
}

VulkanResultMapping mapping(VkResult raw,
                            VulkanCallContext context,
                            RenderOutcome outcome,
                            std::string_view reasonCode,
                            bool setupFailure = false,
                            bool deviceLost = false,
                            bool surfaceInvalid = false) {
  VulkanResultMapping result;
  result.rawResult = raw;
  result.rawResultName = vkResultName(raw);
  result.context = context;
  result.outcome = outcome;
  result.reason = reason(reasonCode);
  result.setupFailure = setupFailure;
  result.deviceLost = deviceLost;
  result.surfaceInvalid = surfaceInvalid;
  return result;
}

}  // namespace

VulkanResultMapping mapVkResult(VkResult result, VulkanCallContext context) {
  switch (result) {
    case VK_SUCCESS:
      return mapping(result, context, RenderOutcome::Ok, "vk_success");
    case VK_NOT_READY:
      return mapping(result, context, RenderOutcome::SkipFrame, "vk_not_ready");
    case VK_TIMEOUT:
      return mapping(result, context, RenderOutcome::SkipFrame, "vk_timeout");
    case VK_SUBOPTIMAL_KHR:
      return mapping(result, context, RenderOutcome::SkipFrame, "swapchain_suboptimal", false,
                     false, true);
    case VK_ERROR_OUT_OF_DATE_KHR:
      return mapping(result, context, RenderOutcome::RecreateSwapchain, "swapchain_out_of_date",
                     false, false, true);
    case VK_ERROR_DEVICE_LOST:
      return mapping(result, context, RenderOutcome::DeviceLost, "device_lost", true, true);
    case VK_ERROR_EXTENSION_NOT_PRESENT:
      return mapping(result, context, RenderOutcome::Unsupported, "unsupported_required_extension",
                     true);
    case VK_ERROR_FEATURE_NOT_PRESENT:
      return mapping(result, context, RenderOutcome::Unsupported, "unsupported_required_feature",
                     true);
    case VK_ERROR_LAYER_NOT_PRESENT:
      return mapping(result, context, RenderOutcome::Unsupported, "validation_layer_missing",
                     true);
    case VK_ERROR_INCOMPATIBLE_DRIVER:
      return mapping(result, context, RenderOutcome::Unsupported, "incompatible_driver", true);
    case VK_ERROR_FORMAT_NOT_SUPPORTED:
      return mapping(result, context, RenderOutcome::Unsupported, "format_not_supported", true);
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
      return mapping(result, context, RenderOutcome::FatalRendererError, "native_window_in_use",
                     true, false, true);
    case VK_ERROR_VALIDATION_FAILED_EXT:
      return mapping(result, context, RenderOutcome::ValidationFailure, "validation_failed", true);
    default:
      return mapping(result, context, RenderOutcome::FatalRendererError, "vulkan_result_unmapped",
                     true);
  }
}

const char* vkResultName(VkResult result) {
  switch (result) {
    case VK_SUCCESS:
      return "VK_SUCCESS";
    case VK_NOT_READY:
      return "VK_NOT_READY";
    case VK_TIMEOUT:
      return "VK_TIMEOUT";
    case VK_SUBOPTIMAL_KHR:
      return "VK_SUBOPTIMAL_KHR";
    case VK_ERROR_OUT_OF_DATE_KHR:
      return "VK_ERROR_OUT_OF_DATE_KHR";
    case VK_ERROR_DEVICE_LOST:
      return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_EXTENSION_NOT_PRESENT:
      return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT:
      return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_LAYER_NOT_PRESENT:
      return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER:
      return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_FORMAT_NOT_SUPPORTED:
      return "VK_ERROR_FORMAT_NOT_SUPPORTED";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
      return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
    case VK_ERROR_VALIDATION_FAILED_EXT:
      return "VK_ERROR_VALIDATION_FAILED_EXT";
    default:
      return "VK_RESULT_UNMAPPED";
  }
}

const char* vulkanCallContextName(VulkanCallContext context) {
  switch (context) {
    case VulkanCallContext::InstanceCreate:
      return "instance_create";
    case VulkanCallContext::SurfaceCreate:
      return "surface_create";
    case VulkanCallContext::PhysicalDeviceEnumerate:
      return "physical_device_enumerate";
    case VulkanCallContext::DeviceCreate:
      return "device_create";
    case VulkanCallContext::QueueRetrieve:
      return "queue_retrieve";
    case VulkanCallContext::FunctionLoad:
      return "function_load";
    case VulkanCallContext::AcquireImage:
      return "acquire_image";
    case VulkanCallContext::Present:
      return "present";
    case VulkanCallContext::QueueSubmit:
      return "queue_submit";
    case VulkanCallContext::FenceWait:
      return "fence_wait";
    case VulkanCallContext::MemoryAllocate:
      return "memory_allocate";
    case VulkanCallContext::ShaderModuleCreate:
      return "shader_module_create";
    case VulkanCallContext::Unknown:
      return "unknown";
  }
  return "unknown";
}

std::vector<std::string_view> packet4VulkanResultReasonCodes() {
  return {"vk_success",
          "vk_not_ready",
          "vk_timeout",
          "swapchain_out_of_date",
          "swapchain_suboptimal",
          "device_lost",
          "unsupported_required_extension",
          "unsupported_required_feature",
          "validation_layer_missing",
          "incompatible_driver",
          "format_not_supported",
          "native_window_in_use",
          "validation_failed",
          "vulkan_result_unmapped"};
}

std::vector<std::string_view> packet4VulkanStartupReasonCodes() {
  return {"vulkan_loader_missing",
          "missing_instance_extension",
          "no_physical_devices",
          "no_suitable_physical_device",
          "missing_graphics_queue",
          "missing_present_queue",
          "missing_swapchain_extension",
          "dynamic_rendering_required_missing",
          "portability_required_missing",
          "software_device_not_allowed",
          "logical_device_create_failed",
          "queue_retrieval_failed",
          "instance_function_missing",
          "device_function_missing",
          "debug_utils_required_missing",
          "debug_messenger_create_failed",
          "vulkan_backend_no_swapchain_yet",
          "vulkan_surface_provider_missing"};
}

std::vector<std::string_view> packet4VulkanSmokeReasonCodes() {
  return {"vulkan_smoke_pass",
          "vulkan_smoke_skipped_loader_missing",
          "vulkan_smoke_skipped_sdl_missing",
          "vulkan_smoke_skipped_no_display",
          "vulkan_smoke_skipped_no_suitable_device",
          "vulkan_smoke_strict_dependency_missing",
          "vulkan_smoke_receipt_invalid",
          "vulkan_smoke_validation_unavailable",
          "vulkan_smoke_validation_failed"};
}

}  // namespace iggy3d::vulkan
