#include "render/vulkan/VulkanFeatureSupport.hpp"

#include <algorithm>
#include <cstring>
#include <set>
#include <sstream>

namespace iggy3d::vulkan {
namespace {

RenderReason featureReason(std::string_view code) {
  if (code == "missing_graphics_queue") {
    return {code, "missing graphics queue"};
  }
  if (code == "missing_present_queue") {
    return {code, "missing present queue"};
  }
  if (code == "missing_swapchain_extension") {
    return {code, "missing swapchain extension"};
  }
  if (code == "dynamic_rendering_required_missing") {
    return {code, "dynamic rendering required missing"};
  }
  if (code == "portability_required_missing") {
    return {code, "portability required missing"};
  }
  if (code == "software_device_not_allowed") {
    return {code, "software device not allowed"};
  }
  return {"unsupported_required_feature", "unsupported required feature"};
}

std::int32_t deviceTypeScore(VkPhysicalDeviceType type) {
  switch (type) {
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
      return 1000;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
      return 500;
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
      return 10;
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
      return 100;
    case VK_PHYSICAL_DEVICE_TYPE_OTHER:
      return 0;
    default:
      return 0;
  }
}

bool hasDedicatedTransfer(const VulkanQueueFamilySelection& queues) {
  return queues.hasTransfer && queues.hasGraphics && queues.transferFamily != queues.graphicsFamily;
}

}  // namespace

VulkanDeviceGateResult evaluateFakeVulkanDeviceGates(
    const FakeVulkanDeviceCandidate& candidate,
    const VulkanFeatureBaselineRequest& request) {
  VulkanDeviceGateResult result;
  result.dynamicRendering.source = "unavailable";
  result.timelineSemaphoreSupported = candidate.timelineSemaphoreSupported;
  result.timelineSemaphoreEnabled = false;
  result.portabilitySubsetRequired = candidate.portabilitySubsetRequired;
  result.portabilitySubsetEnabled =
      candidate.portabilitySubsetRequired && candidate.portabilitySubsetSupported;

  if (!candidate.queues.hasGraphics) {
    result.rejectReasons.push_back(featureReason("missing_graphics_queue"));
  }
  if (request.requirePresentSupport && !candidate.queues.hasPresent) {
    result.rejectReasons.push_back(featureReason("missing_present_queue"));
  }
  if (!candidate.hasSwapchainExtension) {
    result.rejectReasons.push_back(featureReason("missing_swapchain_extension"));
  }
  if (request.requireDynamicRendering) {
    if (candidate.hasDynamicRenderingCore13) {
      result.dynamicRendering.supported = true;
      result.dynamicRendering.enabled = true;
      result.dynamicRendering.source = "core_1_3";
    } else if (request.allowVulkan12DynamicRenderingExtensionPath &&
               candidate.hasDynamicRenderingKhr) {
      result.dynamicRendering.supported = true;
      result.dynamicRendering.enabled = true;
      result.dynamicRendering.source = "khr_extension";
    } else {
      result.rejectReasons.push_back(featureReason("dynamic_rendering_required_missing"));
    }
  }
  if (request.requireDepthFormat && !candidate.hasDepthFormat) {
    result.rejectReasons.push_back(featureReason("unsupported_required_feature"));
  }
  if (candidate.portabilitySubsetRequired && !candidate.portabilitySubsetSupported) {
    result.rejectReasons.push_back(featureReason("portability_required_missing"));
  }
  if (!request.allowSoftwareDevice &&
      candidate.identity.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU) {
    result.rejectReasons.push_back(featureReason("software_device_not_allowed"));
  }

  result.passed = result.rejectReasons.empty();
  return result;
}

VulkanDeviceScore scoreVulkanDevice(const VulkanDeviceIdentity& identity,
                                    const VulkanQueueFamilySelection& queues,
                                    const VulkanDeviceGateResult& gates) {
  VulkanDeviceScore score;
  if (!gates.passed) {
    score.scoreBreakdown = "failed_hard_gates";
    return score;
  }

  score.value += deviceTypeScore(identity.deviceType);
  std::ostringstream breakdown;
  breakdown << "device_type=" << deviceTypeScore(identity.deviceType);
  if (queues.graphicsAndPresentSame) {
    score.value += 50;
    breakdown << ",same_graphics_present=50";
  }
  if (hasDedicatedTransfer(queues)) {
    score.value += 10;
    breakdown << ",dedicated_transfer=10";
  }
  score.scoreBreakdown = breakdown.str();
  return score;
}

FakeVulkanDeviceSelectionResult selectFakeVulkanDevice(
    const std::vector<FakeVulkanDeviceCandidate>& candidates,
    const VulkanFeatureBaselineRequest& request) {
  FakeVulkanDeviceSelectionResult result;
  if (candidates.empty()) {
    result.reason = {"no_physical_devices", "no physical devices"};
    return result;
  }

  bool sawRejected = false;
  for (std::size_t index = 0; index < candidates.size(); ++index) {
    const VulkanDeviceGateResult gates = evaluateFakeVulkanDeviceGates(candidates[index], request);
    const VulkanDeviceScore score =
        scoreVulkanDevice(candidates[index].identity, candidates[index].queues, gates);
    if (!gates.passed) {
      sawRejected = true;
      if (result.gates.rejectReasons.empty()) {
        result.gates = gates;
      }
      continue;
    }
    if (!result.selected || score.value > result.score.value ||
        (score.value == result.score.value &&
         candidates[index].identity.name < candidates[result.selectedIndex].identity.name) ||
        (score.value == result.score.value &&
         candidates[index].identity.name == candidates[result.selectedIndex].identity.name &&
         candidates[index].stableIndex < candidates[result.selectedIndex].stableIndex)) {
      result.selected = true;
      result.selectedIndex = index;
      result.gates = gates;
      result.score = score;
      result.reason = {"vulkan_smoke_pass", "vulkan smoke pass"};
    }
  }
  if (!result.selected && sawRejected) {
    result.reason = {"no_suitable_physical_device", "no suitable physical device"};
  }
  return result;
}

std::string formatDeviceRejectionReasons(const std::vector<RenderReason>& reasons) {
  std::string formatted;
  for (std::size_t i = 0; i < reasons.size(); ++i) {
    if (i != 0U) {
      formatted.push_back(',');
    }
    formatted += reasons[i].code;
  }
  return formatted;
}

#if defined(IGGY3D_HAS_VULKAN)
VulkanQueueFamilySelection queryQueueFamilies(VkPhysicalDevice physicalDevice,
                                              VkSurfaceKHR surface,
                                              bool requirePresentSupport) {
  VulkanQueueFamilySelection result;
  std::uint32_t count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);
  std::vector<VkQueueFamilyProperties> families(count);
  if (count != 0U) {
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, families.data());
  }

  for (std::uint32_t i = 0; i < count; ++i) {
    const VkQueueFlags flags = families[i].queueFlags;
    if (!result.hasGraphics && (flags & VK_QUEUE_GRAPHICS_BIT) != 0U) {
      result.hasGraphics = true;
      result.graphicsFamily = i;
    }
    if (!result.hasCompute && (flags & VK_QUEUE_COMPUTE_BIT) != 0U) {
      result.hasCompute = true;
      result.computeFamily = i;
    }
    if (!result.hasTransfer && (flags & VK_QUEUE_TRANSFER_BIT) != 0U) {
      result.hasTransfer = true;
      result.transferFamily = i;
    }
    if (requirePresentSupport && surface != VK_NULL_HANDLE) {
      VkBool32 present = VK_FALSE;
      const VkResult presentResult =
          vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &present);
      if (presentResult == VK_SUCCESS && present == VK_TRUE && !result.hasPresent) {
        result.hasPresent = true;
        result.presentFamily = i;
      }
    }
  }
  if (!requirePresentSupport && result.hasGraphics) {
    result.hasPresent = true;
    result.presentFamily = result.graphicsFamily;
  }
  result.graphicsAndPresentSame = result.hasGraphics && result.hasPresent &&
                                  result.graphicsFamily == result.presentFamily;
  return result;
}

VulkanDeviceIdentity queryDeviceIdentity(VkPhysicalDevice physicalDevice) {
  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(physicalDevice, &properties);
  VulkanDeviceIdentity identity;
  identity.name = properties.deviceName;
  identity.vendorId = properties.vendorID;
  identity.deviceId = properties.deviceID;
  identity.deviceType = properties.deviceType;
  identity.apiVersion = properties.apiVersion;
  identity.driverVersion = properties.driverVersion;
  return identity;
}

FakeVulkanDeviceCandidate queryPhysicalDeviceCandidate(
    VkPhysicalDevice physicalDevice,
    VkSurfaceKHR surface,
    const VulkanFeatureBaselineRequest& request) {
  FakeVulkanDeviceCandidate candidate;
  candidate.identity = queryDeviceIdentity(physicalDevice);
  candidate.queues = queryQueueFamilies(physicalDevice, surface, request.requirePresentSupport);
  candidate.hasDepthFormat = true;
  candidate.hasSwapchainExtension = false;
  candidate.hasDynamicRenderingCore13 = false;
  candidate.hasDynamicRenderingKhr = false;

  std::uint32_t extensionCount = 0;
  vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
  std::vector<VkExtensionProperties> extensions(extensionCount);
  if (extensionCount != 0U) {
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount,
                                         extensions.data());
  }
  for (const VkExtensionProperties& extension : extensions) {
    if (std::strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
      candidate.hasSwapchainExtension = true;
    }
    if (std::strcmp(extension.extensionName, "VK_KHR_dynamic_rendering") == 0) {
      candidate.hasDynamicRenderingKhr = true;
    }
    if (std::strcmp(extension.extensionName, "VK_KHR_portability_subset") == 0) {
      candidate.portabilitySubsetRequired = true;
      candidate.portabilitySubsetSupported = true;
    }
  }
  candidate.hasDynamicRenderingCore13 = candidate.identity.apiVersion >= VK_API_VERSION_1_3;
  candidate.timelineSemaphoreSupported = false;
  return candidate;
}
#endif

}  // namespace iggy3d::vulkan
