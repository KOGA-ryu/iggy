#include "render/RenderDiagnostics.hpp"
#include "render/vulkan/VulkanFeatureSupport.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

bool strictLane() {
#if defined(IGGY3D_REQUIRE_VULKAN_SMOKE_ENABLED)
  return true;
#else
  return false;
#endif
}

iggy3d::RenderReceipt baseReceipt(std::string_view result, std::string_view reason) {
  iggy3d::RenderReceipt receipt;
  iggy3d::appendReceiptField(receipt, "receipt_version", "1");
  iggy3d::appendReceiptField(receipt, "repo", "iggy3d");
  iggy3d::appendReceiptField(receipt, "smoke", "vulkan_feature_baseline");
  iggy3d::appendReceiptField(receipt, "api_baseline", "vulkan_1_3_preferred");
  iggy3d::appendReceiptField(receipt, "api_compatibility_floor",
                             "vulkan_1_2_plus_required_extensions");
  iggy3d::appendReceiptField(receipt, "physical_device_count", static_cast<std::uint64_t>(0));
  iggy3d::appendReceiptField(receipt, "selected_physical_device_index", "none");
  iggy3d::appendReceiptField(receipt, "device_name", "none");
  iggy3d::appendReceiptField(receipt, "device_type", "none");
  iggy3d::appendReceiptField(receipt, "api_version_selected", "none");
  iggy3d::appendReceiptField(receipt, "dynamic_rendering_required", true);
  iggy3d::appendReceiptField(receipt, "dynamic_rendering_supported", false);
  iggy3d::appendReceiptField(receipt, "dynamic_rendering_enabled", false);
  iggy3d::appendReceiptField(receipt, "dynamic_rendering_source", "unavailable");
  iggy3d::appendReceiptField(receipt, "timeline_semaphore_supported", "unavailable");
  iggy3d::appendReceiptField(receipt, "timeline_semaphore_enabled", false);
  iggy3d::appendReceiptField(receipt, "swapchain_extension_supported", false);
  iggy3d::appendReceiptField(receipt, "min_push_constant_bytes_required",
                             static_cast<std::uint64_t>(64));
  iggy3d::appendReceiptField(receipt, "min_push_constant_bytes_supported", "none");
  iggy3d::appendReceiptField(receipt, "depth_format_supported", "unavailable");
  iggy3d::appendReceiptField(receipt, "portability_enumeration", "unavailable");
  iggy3d::appendReceiptField(receipt, "portability_subset", "unavailable");
  iggy3d::appendReceiptField(receipt, "surface_required", false);
  iggy3d::appendReceiptField(receipt, "strict_lane", strictLane());
  iggy3d::appendReceiptField(receipt, "result", result);
  iggy3d::appendReceiptField(receipt, "reason_code", reason);
  return receipt;
}

int printReceipt(const iggy3d::RenderReceipt& receipt, int exitCode) {
  std::cout << iggy3d::formatRenderReceipt(receipt);
  return exitCode;
}

#if defined(IGGY3D_HAS_VULKAN)
bool instanceExtensionAvailable(const char* requested) {
  std::uint32_t count = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
  std::vector<VkExtensionProperties> extensions(count);
  if (count != 0U) {
    vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data());
  }
  return std::any_of(extensions.begin(), extensions.end(),
                     [requested](const VkExtensionProperties& extension) {
                       return std::strcmp(extension.extensionName, requested) == 0;
                     });
}

std::vector<const char*> instanceExtensions() {
  std::vector<const char*> extensions;
#if defined(__APPLE__)
  if (instanceExtensionAvailable(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
  }
#endif
  return extensions;
}

iggy3d::RenderReceipt featureReceipt(
    const std::vector<iggy3d::vulkan::FakeVulkanDeviceCandidate>& candidates,
    const iggy3d::vulkan::FakeVulkanDeviceSelectionResult& selected) {
  iggy3d::RenderReceipt receipt;
  iggy3d::appendReceiptField(receipt, "receipt_version", "1");
  iggy3d::appendReceiptField(receipt, "repo", "iggy3d");
  iggy3d::appendReceiptField(receipt, "smoke", "vulkan_feature_baseline");
  iggy3d::appendReceiptField(receipt, "api_baseline", "vulkan_1_3_preferred");
  iggy3d::appendReceiptField(receipt, "api_compatibility_floor",
                             "vulkan_1_2_plus_required_extensions");
  iggy3d::appendReceiptField(receipt, "physical_device_count",
                             static_cast<std::uint64_t>(candidates.size()));
  iggy3d::appendReceiptField(receipt, "selected_physical_device_index",
                             static_cast<std::uint64_t>(selected.selectedIndex));
  const iggy3d::vulkan::FakeVulkanDeviceCandidate& candidate =
      candidates[selected.selectedIndex];
  iggy3d::appendReceiptField(receipt, "device_name", candidate.identity.name);
  iggy3d::appendReceiptField(receipt, "device_type",
                             iggy3d::vulkan::vulkanDeviceTypeName(candidate.identity.deviceType));
  iggy3d::appendReceiptField(
      receipt, "api_version_selected",
      iggy3d::vulkan::formatVulkanApiVersion(candidate.identity.apiVersion));
  iggy3d::appendReceiptField(receipt, "dynamic_rendering_supported",
                             selected.gates.dynamicRendering.supported);
  iggy3d::appendReceiptField(receipt, "dynamic_rendering_enabled",
                             selected.gates.dynamicRendering.enabled);
  iggy3d::appendReceiptField(receipt, "dynamic_rendering_source",
                             selected.gates.dynamicRendering.source);
  iggy3d::appendReceiptField(receipt, "timeline_semaphore_supported",
                             selected.gates.timelineSemaphoreSupported);
  iggy3d::appendReceiptField(receipt, "timeline_semaphore_enabled",
                             selected.gates.timelineSemaphoreEnabled);
  iggy3d::appendReceiptField(receipt, "swapchain_extension_supported",
                             candidate.hasSwapchainExtension);
  iggy3d::appendReceiptField(receipt, "min_push_constant_bytes_supported", "unqueried");
  iggy3d::appendReceiptField(receipt, "depth_format_supported", candidate.hasDepthFormat);
  iggy3d::appendReceiptField(receipt, "portability_enumeration", "queried");
  iggy3d::appendReceiptField(receipt, "portability_subset",
                             selected.gates.portabilitySubsetEnabled ? "enabled"
                                                                     : "not_required");
  iggy3d::appendReceiptField(receipt, "surface_required", false);
  iggy3d::appendReceiptField(receipt, "strict_lane", strictLane());
  iggy3d::appendReceiptField(receipt, "result", "pass");
  iggy3d::appendReceiptField(receipt, "reason_code", "vulkan_smoke_pass");
  return receipt;
}
#endif

}  // namespace

int main() {
#if !defined(IGGY3D_HAS_VULKAN)
  return printReceipt(baseReceipt(strictLane() ? "fail" : "skip",
                                  strictLane() ? "vulkan_smoke_strict_dependency_missing"
                                               : "vulkan_smoke_skipped_loader_missing"),
                      strictLane() ? 1 : 77);
#else
  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "iggy3d feature baseline smoke";
  appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
  appInfo.pEngineName = "iggy3d";
  appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
  appInfo.apiVersion = VK_API_VERSION_1_3;

  const std::vector<const char*> extensions = instanceExtensions();
  VkInstanceCreateInfo instanceInfo{};
  instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instanceInfo.pApplicationInfo = &appInfo;
  instanceInfo.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
  instanceInfo.ppEnabledExtensionNames = extensions.data();
#if defined(__APPLE__)
  if (!extensions.empty()) {
    instanceInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
  }
#endif

  VkInstance instance = VK_NULL_HANDLE;
  if (vkCreateInstance(&instanceInfo, nullptr, &instance) != VK_SUCCESS) {
    return printReceipt(baseReceipt(strictLane() ? "fail" : "skip",
                                    strictLane() ? "vulkan_smoke_strict_dependency_missing"
                                                 : "vulkan_smoke_skipped_loader_missing"),
                        strictLane() ? 1 : 77);
  }

  std::uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
  std::vector<VkPhysicalDevice> devices(deviceCount);
  if (deviceCount != 0U) {
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
  }
  if (deviceCount == 0U) {
    vkDestroyInstance(instance, nullptr);
    return printReceipt(baseReceipt(strictLane() ? "fail" : "skip",
                                    strictLane() ? "vulkan_smoke_strict_dependency_missing"
                                                 : "vulkan_smoke_skipped_no_suitable_device"),
                        strictLane() ? 1 : 77);
  }

  iggy3d::vulkan::VulkanFeatureBaselineRequest request;
  request.requirePresentSupport = false;
  std::vector<iggy3d::vulkan::FakeVulkanDeviceCandidate> candidates;
  candidates.reserve(devices.size());
  for (std::size_t i = 0; i < devices.size(); ++i) {
    iggy3d::vulkan::FakeVulkanDeviceCandidate candidate =
        iggy3d::vulkan::queryPhysicalDeviceCandidate(devices[i], VK_NULL_HANDLE, request);
    candidate.stableIndex = static_cast<std::int32_t>(i);
    candidates.push_back(candidate);
  }

  const iggy3d::vulkan::FakeVulkanDeviceSelectionResult selected =
      iggy3d::vulkan::selectFakeVulkanDevice(candidates, request);
  if (!selected.selected) {
    iggy3d::RenderReceipt receipt =
        baseReceipt(strictLane() ? "fail" : "skip",
                    strictLane() ? "vulkan_smoke_strict_dependency_missing"
                                 : "vulkan_smoke_skipped_no_suitable_device");
    iggy3d::appendReceiptField(receipt, "physical_device_count",
                               static_cast<std::uint64_t>(candidates.size()));
    iggy3d::appendReceiptField(
        receipt, "device_reject_reasons",
        iggy3d::vulkan::formatDeviceRejectionReasons(selected.gates.rejectReasons));
    vkDestroyInstance(instance, nullptr);
    return printReceipt(receipt, strictLane() ? 1 : 77);
  }

  const iggy3d::RenderReceipt receipt = featureReceipt(candidates, selected);
  vkDestroyInstance(instance, nullptr);
  return printReceipt(receipt, 0);
#endif
}
