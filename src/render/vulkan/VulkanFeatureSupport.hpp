#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "render/RenderDiagnostics.hpp"
#include "render/vulkan/VulkanTypes.hpp"

namespace iggy3d::vulkan {

struct VulkanFeatureBaselineRequest {
  std::uint32_t requestedApiVersion = (1U << 22U) | (3U << 12U);
  bool requireDynamicRendering = true;
  bool allowVulkan12DynamicRenderingExtensionPath = true;
  bool allowSoftwareDevice = false;
  bool requirePresentSupport = true;
  bool requireDepthFormat = true;
};

struct VulkanDeviceGateResult {
  bool passed = false;
  std::vector<RenderReason> rejectReasons;
  VulkanDynamicRenderingDecision dynamicRendering;
  bool timelineSemaphoreSupported = false;
  bool timelineSemaphoreEnabled = false;
  bool portabilitySubsetRequired = false;
  bool portabilitySubsetEnabled = false;
};

struct VulkanDeviceScore {
  std::int32_t value = 0;
  std::string scoreBreakdown;
};

struct FakeVulkanDeviceCandidate {
  VulkanDeviceIdentity identity;
  VulkanQueueFamilySelection queues;
  bool hasSwapchainExtension = true;
  bool hasDynamicRenderingCore13 = true;
  bool hasDynamicRenderingKhr = true;
  bool hasDepthFormat = true;
  bool portabilitySubsetRequired = false;
  bool portabilitySubsetSupported = false;
  bool timelineSemaphoreSupported = true;
  std::int32_t stableIndex = 0;
};

struct FakeVulkanDeviceSelectionResult {
  bool selected = false;
  std::size_t selectedIndex = 0;
  VulkanDeviceGateResult gates;
  VulkanDeviceScore score;
  RenderReason reason{"no_suitable_physical_device", "no suitable physical device"};
};

VulkanDeviceGateResult evaluateFakeVulkanDeviceGates(
    const FakeVulkanDeviceCandidate& candidate,
    const VulkanFeatureBaselineRequest& request);

VulkanDeviceScore scoreVulkanDevice(const VulkanDeviceIdentity& identity,
                                    const VulkanQueueFamilySelection& queues,
                                    const VulkanDeviceGateResult& gates);

FakeVulkanDeviceSelectionResult selectFakeVulkanDevice(
    const std::vector<FakeVulkanDeviceCandidate>& candidates,
    const VulkanFeatureBaselineRequest& request);

std::string formatDeviceRejectionReasons(const std::vector<RenderReason>& reasons);

#if defined(IGGY3D_HAS_VULKAN)
VulkanQueueFamilySelection queryQueueFamilies(VkPhysicalDevice physicalDevice,
                                              VkSurfaceKHR surface,
                                              bool requirePresentSupport);
VulkanDeviceIdentity queryDeviceIdentity(VkPhysicalDevice physicalDevice);
FakeVulkanDeviceCandidate queryPhysicalDeviceCandidate(VkPhysicalDevice physicalDevice,
                                                       VkSurfaceKHR surface,
                                                       const VulkanFeatureBaselineRequest& request);
#endif

}  // namespace iggy3d::vulkan
