#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "render/RenderDiagnostics.hpp"

#if defined(IGGY3D_HAS_VULKAN)
#include <vulkan/vulkan.h>
#else
using VkResult = int;
constexpr VkResult VK_SUCCESS = 0;
constexpr VkResult VK_NOT_READY = 1;
constexpr VkResult VK_TIMEOUT = 2;
constexpr VkResult VK_ERROR_OUT_OF_HOST_MEMORY = -1;
constexpr VkResult VK_ERROR_OUT_OF_DEVICE_MEMORY = -2;
constexpr VkResult VK_ERROR_INITIALIZATION_FAILED = -3;
constexpr VkResult VK_ERROR_DEVICE_LOST = -4;
constexpr VkResult VK_ERROR_EXTENSION_NOT_PRESENT = -7;
constexpr VkResult VK_ERROR_FEATURE_NOT_PRESENT = -8;
constexpr VkResult VK_ERROR_LAYER_NOT_PRESENT = -6;
constexpr VkResult VK_ERROR_INCOMPATIBLE_DRIVER = -9;
constexpr VkResult VK_ERROR_FORMAT_NOT_SUPPORTED = -11;
constexpr VkResult VK_ERROR_VALIDATION_FAILED_EXT = -1000011001;
constexpr VkResult VK_ERROR_OUT_OF_DATE_KHR = -1000001004;
constexpr VkResult VK_SUBOPTIMAL_KHR = 1000001003;
constexpr VkResult VK_ERROR_NATIVE_WINDOW_IN_USE_KHR = -1000000001;
#endif

namespace iggy3d::vulkan {

enum class VulkanCallContext : std::uint8_t {
  InstanceCreate,
  SurfaceCreate,
  PhysicalDeviceEnumerate,
  DeviceCreate,
  QueueRetrieve,
  FunctionLoad,
  AcquireImage,
  Present,
  QueueSubmit,
  FenceWait,
  MemoryAllocate,
  ShaderModuleCreate,
  Unknown,
};

struct VulkanResultMapping {
  VkResult rawResult = VK_SUCCESS;
  const char* rawResultName = "VK_SUCCESS";
  VulkanCallContext context = VulkanCallContext::Unknown;
  RenderOutcome outcome = RenderOutcome::Ok;
  RenderReason reason{"vk_success", "vulkan success"};
  bool setupFailure = false;
  bool deviceLost = false;
  bool surfaceInvalid = false;
};

VulkanResultMapping mapVkResult(VkResult result, VulkanCallContext context);
const char* vkResultName(VkResult result);

std::vector<std::string_view> packet4VulkanResultReasonCodes();
std::vector<std::string_view> packet4VulkanStartupReasonCodes();
std::vector<std::string_view> packet4VulkanSmokeReasonCodes();
std::vector<std::string_view> packet6VulkanReasonCodes();

}  // namespace iggy3d::vulkan
