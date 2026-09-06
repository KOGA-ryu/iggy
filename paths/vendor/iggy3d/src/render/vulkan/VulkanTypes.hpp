#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#if defined(IGGY3D_HAS_VULKAN)
#include <vulkan/vulkan.h>
#else
using VkInstance = void*;
using VkSurfaceKHR = void*;
using VkPhysicalDevice = void*;
using VkDevice = void*;
using VkQueue = void*;
enum VkPhysicalDeviceType : std::uint32_t {
  VK_PHYSICAL_DEVICE_TYPE_OTHER = 0,
  VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU = 1,
  VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU = 2,
  VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU = 3,
  VK_PHYSICAL_DEVICE_TYPE_CPU = 4,
};
#endif

namespace iggy3d::vulkan {

constexpr std::uint32_t kInvalidVulkanQueueFamily = UINT32_MAX;

struct VulkanQueueFamilySelection {
  std::uint32_t graphicsFamily = kInvalidVulkanQueueFamily;
  std::uint32_t presentFamily = kInvalidVulkanQueueFamily;
  std::uint32_t computeFamily = kInvalidVulkanQueueFamily;
  std::uint32_t transferFamily = kInvalidVulkanQueueFamily;
  bool hasGraphics = false;
  bool hasPresent = false;
  bool hasCompute = false;
  bool hasTransfer = false;
  bool graphicsAndPresentSame = false;
};

struct VulkanDeviceIdentity {
  std::string name;
  std::uint32_t vendorId = 0;
  std::uint32_t deviceId = 0;
  VkPhysicalDeviceType deviceType = VK_PHYSICAL_DEVICE_TYPE_OTHER;
  std::uint32_t apiVersion = 0;
  std::uint32_t driverVersion = 0;
};

struct VulkanBootstrapHandles {
  VkInstance instance{};
  VkSurfaceKHR surface{};
  VkPhysicalDevice physicalDevice{};
  VkDevice device{};
  VkQueue graphicsQueue{};
  VkQueue presentQueue{};
};

struct VulkanDynamicRenderingDecision {
  bool supported = false;
  bool enabled = false;
  std::string source = "unavailable";
};

inline std::string_view vulkanDeviceTypeName(VkPhysicalDeviceType type) {
  switch (type) {
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
      return "discrete";
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
      return "integrated";
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
      return "cpu";
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
      return "virtual";
    case VK_PHYSICAL_DEVICE_TYPE_OTHER:
      return "other";
    default:
      return "other";
  }
}

inline std::string formatVulkanApiVersion(std::uint32_t version) {
  const std::uint32_t major = version >> 22U;
  const std::uint32_t minor = (version >> 12U) & 0x3FFU;
  const std::uint32_t patch = version & 0xFFFU;
  return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
}

}  // namespace iggy3d::vulkan
