#include "render/vulkan/InstanceDeviceSurface.hpp"

#include <algorithm>
#include <cstring>
#include <set>
#include <sstream>

namespace iggy3d::vulkan {
namespace {

void appendBootstrapBase(RenderReceipt& receipt, std::string_view reasonCode) {
  appendReceiptField(receipt, "receipt_version", "1");
  appendReceiptField(receipt, "repo", "iggy3d");
  appendReceiptField(receipt, "file_plan", "src/render/vulkan/InstanceDeviceSurface.cpp");
  appendReceiptField(receipt, "packet_order", "4");
  appendReceiptField(receipt, "allowed_to_implement_code_now", "false");
  appendReceiptField(receipt, "backend", "vulkan");
  appendReceiptField(receipt, "reason_code", reasonCode);
}

std::string joinStrings(const std::vector<std::string>& values) {
  std::string joined;
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0U) {
      joined.push_back(',');
    }
    joined += values[i];
  }
  return joined;
}

RenderReceipt failureReceipt(std::string_view reasonCode) {
  RenderReceipt receipt;
  appendBootstrapBase(receipt, reasonCode);
  appendReceiptField(receipt, "physical_device_count", static_cast<std::uint64_t>(0));
  appendReceiptField(receipt, "selected_physical_device_index", "none");
  appendReceiptField(receipt, "device_name", "none");
  appendReceiptField(receipt, "device_type", "none");
  appendReceiptField(receipt, "api_version_selected", "none");
  appendReceiptField(receipt, "driver_version", "none");
  appendReceiptField(receipt, "graphics_queue_family", "none");
  appendReceiptField(receipt, "present_queue_family", "none");
  appendReceiptField(receipt, "enabled_instance_extensions", "");
  appendReceiptField(receipt, "enabled_device_extensions", "");
  appendReceiptField(receipt, "enabled_features", "");
  appendReceiptField(receipt, "function_loading_clean", false);
  appendReceiptField(receipt, "result", "fail");
  return receipt;
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

std::vector<const char*> toCStringList(const std::vector<std::string>& names) {
  std::vector<const char*> list;
  list.reserve(names.size());
  for (const std::string& name : names) {
    list.push_back(name.c_str());
  }
  return list;
}
#endif

}  // namespace

InstanceDeviceSurface::~InstanceDeviceSurface() {
  shutdown();
}

RenderReceipt InstanceDeviceSurface::initialize(const InstanceDeviceSurfaceCreateInfo& createInfo) {
  shutdown();
  if (createInfo.surfaceProvider.createSurface == nullptr) {
    lastReceipt_ = failureReceipt("vulkan_surface_provider_missing");
    return lastReceipt_;
  }

  DebugValidationPlan validationPlan = validation_.buildPlan(createInfo.validation);
  if (hasReceiptField(validationPlan.receipt, "result", "fail")) {
    lastReceipt_ = validationPlan.receipt;
    return lastReceipt_;
  }

#if !defined(IGGY3D_HAS_VULKAN)
  lastReceipt_ = failureReceipt("vulkan_loader_missing");
  return lastReceipt_;
#else
  std::vector<std::string> instanceExtensions = createInfo.surfaceProvider.requiredInstanceExtensions;
  for (const char* extension : validationPlan.instanceExtensions) {
    instanceExtensions.emplace_back(extension);
  }
#if defined(__APPLE__)
  if (instanceExtensionAvailable(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
    instanceExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
  }
#endif
  std::sort(instanceExtensions.begin(), instanceExtensions.end());
  instanceExtensions.erase(std::unique(instanceExtensions.begin(), instanceExtensions.end()),
                           instanceExtensions.end());

  for (const std::string& extension : instanceExtensions) {
    if (!instanceExtensionAvailable(extension.c_str())) {
      lastReceipt_ = failureReceipt("missing_instance_extension");
      appendReceiptField(lastReceipt_, "missing_required_extensions", extension);
      return lastReceipt_;
    }
  }

  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "iggy3d";
  appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
  appInfo.pEngineName = "iggy3d";
  appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
  appInfo.apiVersion = VK_API_VERSION_1_3;

  const std::vector<const char*> extensionNames = toCStringList(instanceExtensions);
  VkInstanceCreateInfo instanceInfo{};
  instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instanceInfo.pApplicationInfo = &appInfo;
  instanceInfo.enabledExtensionCount = static_cast<std::uint32_t>(extensionNames.size());
  instanceInfo.ppEnabledExtensionNames = extensionNames.data();
  instanceInfo.enabledLayerCount = static_cast<std::uint32_t>(validationPlan.instanceLayers.size());
  instanceInfo.ppEnabledLayerNames = validationPlan.instanceLayers.data();
#if defined(__APPLE__)
  instanceInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

  if (vkCreateInstance(&instanceInfo, nullptr, &handles_.instance) != VK_SUCCESS) {
    lastReceipt_ = failureReceipt("incompatible_driver");
    return lastReceipt_;
  }

  validation_.createMessenger(handles_.instance, validationPlan);
  RenderReceipt surfaceReceipt =
      createInfo.surfaceProvider.createSurface(handles_.instance, &handles_.surface);
  if (handles_.surface == VK_NULL_HANDLE ||
      hasReceiptField(surfaceReceipt, "result", "fail") ||
      hasReceiptField(surfaceReceipt, "result", "skip")) {
    lastReceipt_ = failureReceipt("missing_present_queue");
    releaseHandles();
    return lastReceipt_;
  }

  std::uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(handles_.instance, &deviceCount, nullptr);
  if (deviceCount == 0U) {
    lastReceipt_ = failureReceipt("no_physical_devices");
    releaseHandles();
    return lastReceipt_;
  }
  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(handles_.instance, &deviceCount, devices.data());

  std::vector<FakeVulkanDeviceCandidate> candidates;
  candidates.reserve(devices.size());
  for (std::size_t i = 0; i < devices.size(); ++i) {
    FakeVulkanDeviceCandidate candidate =
        queryPhysicalDeviceCandidate(devices[i], handles_.surface, createInfo.featureRequest);
    candidate.stableIndex = static_cast<std::int32_t>(i);
    candidates.push_back(candidate);
  }
  FakeVulkanDeviceSelectionResult selected =
      selectFakeVulkanDevice(candidates, createInfo.featureRequest);
  if (!selected.selected) {
    lastReceipt_ = failureReceipt("no_suitable_physical_device");
    appendReceiptField(lastReceipt_, "physical_device_count",
                       static_cast<std::uint64_t>(deviceCount));
    appendReceiptField(lastReceipt_, "device_reject_reasons",
                       formatDeviceRejectionReasons(selected.gates.rejectReasons));
    releaseHandles();
    return lastReceipt_;
  }

  handles_.physicalDevice = devices[selected.selectedIndex];
  selectedDevice_ = candidates[selected.selectedIndex].identity;
  queues_ = candidates[selected.selectedIndex].queues;

  std::set<std::uint32_t> uniqueFamilies{queues_.graphicsFamily, queues_.presentFamily};
  const float priority = 1.0F;
  std::vector<VkDeviceQueueCreateInfo> queueInfos;
  for (const std::uint32_t family : uniqueFamilies) {
    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = family;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority;
    queueInfos.push_back(queueInfo);
  }

  std::vector<const char*> deviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  if (candidates[selected.selectedIndex].portabilitySubsetSupported) {
    deviceExtensions.push_back("VK_KHR_portability_subset");
  }

  VkPhysicalDeviceDynamicRenderingFeatures dynamicRendering{};
  dynamicRendering.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
  dynamicRendering.dynamicRendering = VK_TRUE;

  VkDeviceCreateInfo deviceInfo{};
  deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceInfo.pNext = &dynamicRendering;
  deviceInfo.queueCreateInfoCount = static_cast<std::uint32_t>(queueInfos.size());
  deviceInfo.pQueueCreateInfos = queueInfos.data();
  deviceInfo.enabledExtensionCount = static_cast<std::uint32_t>(deviceExtensions.size());
  deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();

  if (vkCreateDevice(handles_.physicalDevice, &deviceInfo, nullptr, &handles_.device) !=
      VK_SUCCESS) {
    lastReceipt_ = failureReceipt("logical_device_create_failed");
    releaseHandles();
    return lastReceipt_;
  }
  vkGetDeviceQueue(handles_.device, queues_.graphicsFamily, 0, &handles_.graphicsQueue);
  vkGetDeviceQueue(handles_.device, queues_.presentFamily, 0, &handles_.presentQueue);
  if (handles_.graphicsQueue == VK_NULL_HANDLE || handles_.presentQueue == VK_NULL_HANDLE) {
    lastReceipt_ = failureReceipt("queue_retrieval_failed");
    releaseHandles();
    return lastReceipt_;
  }

  VulkanFunctionLoadConfig functionConfig;
  functionConfig.debugUtilsEnabled = validationPlan.debugUtilsFound;
  functionConfig.debugUtilsRequired = validationPlan.debugUtilsRequired;
  functionConfig.swapchainEnabled = true;
  functionConfig.dynamicRenderingCore13 =
      selected.gates.dynamicRendering.source == "core_1_3";
  functionConfig.dynamicRenderingKhr =
      selected.gates.dynamicRendering.source == "khr_extension";
  functions_ = loadVulkanFunctions(handles_.instance, handles_.device, functionConfig);

  ready_ = functions_.clean;
  RenderReceipt receipt;
  appendBootstrapBase(receipt, ready_ ? "vulkan_smoke_pass" : "device_function_missing");
  appendReceiptField(receipt, "physical_device_count", static_cast<std::uint64_t>(deviceCount));
  appendReceiptField(receipt, "selected_physical_device_index",
                     static_cast<std::uint64_t>(selected.selectedIndex));
  appendReceiptField(receipt, "device_name", selectedDevice_.name);
  appendReceiptField(receipt, "device_vendor_id", static_cast<std::uint64_t>(selectedDevice_.vendorId));
  appendReceiptField(receipt, "device_id", static_cast<std::uint64_t>(selectedDevice_.deviceId));
  appendReceiptField(receipt, "device_type", vulkanDeviceTypeName(selectedDevice_.deviceType));
  appendReceiptField(receipt, "api_version_selected",
                     formatVulkanApiVersion(selectedDevice_.apiVersion));
  appendReceiptField(receipt, "driver_version", static_cast<std::uint64_t>(selectedDevice_.driverVersion));
  appendReceiptField(receipt, "device_score", static_cast<std::uint64_t>(selected.score.value));
  appendReceiptField(receipt, "graphics_queue_family", static_cast<std::uint64_t>(queues_.graphicsFamily));
  appendReceiptField(receipt, "present_queue_family", static_cast<std::uint64_t>(queues_.presentFamily));
  appendReceiptField(receipt, "enabled_instance_extensions", joinStrings(instanceExtensions));
  appendReceiptField(receipt, "enabled_device_extensions", "VK_KHR_swapchain");
  appendReceiptField(receipt, "enabled_features", "dynamic_rendering");
  appendReceiptField(receipt, "missing_required_extensions", "");
  appendReceiptField(receipt, "missing_required_features", "");
  appendReceiptField(receipt, "dynamic_rendering_source", selected.gates.dynamicRendering.source);
  appendReceiptField(receipt, "validation",
                     validationPlan.validationLayerFound ? "enabled"
                                                        : (validationPlan.validationLayerRequested
                                                               ? "unavailable"
                                                               : "disabled"));
  appendReceiptField(receipt, "sync_validation",
                     validationPlan.syncValidationRequested
                         ? (validationPlan.validationLayerFound ? "enabled" : "unavailable")
                         : "disabled");
  appendReceiptField(receipt, "function_loading_clean", functions_.clean);
  appendReceiptField(receipt, "result", ready_ ? "pass" : "fail");
  lastReceipt_ = receipt;
  return lastReceipt_;
#endif
}

void InstanceDeviceSurface::releaseHandles() {
#if defined(IGGY3D_HAS_VULKAN)
  if (handles_.device != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(handles_.device);
    vkDestroyDevice(handles_.device, nullptr);
  }
  if (handles_.surface != VK_NULL_HANDLE && handles_.instance != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(handles_.instance, handles_.surface, nullptr);
  }
  validation_.destroyMessenger(handles_.instance);
  if (handles_.instance != VK_NULL_HANDLE) {
    vkDestroyInstance(handles_.instance, nullptr);
  }
#endif
  handles_ = {};
  queues_ = {};
  selectedDevice_ = {};
  functions_ = {};
  ready_ = false;
}

RenderReceipt InstanceDeviceSurface::shutdown() {
  releaseHandles();
  RenderReceipt receipt;
  appendBootstrapBase(receipt, "vulkan_smoke_pass");
  appendReceiptField(receipt, "result", "pass");
  appendReceiptField(receipt, "shutdown", true);
  lastReceipt_ = receipt;
  return lastReceipt_;
}

const VulkanBootstrapHandles& InstanceDeviceSurface::handles() const {
  return handles_;
}

const VulkanQueueFamilySelection& InstanceDeviceSurface::queues() const {
  return queues_;
}

const VulkanDeviceIdentity& InstanceDeviceSurface::selectedDevice() const {
  return selectedDevice_;
}

const VulkanFunctionTables& InstanceDeviceSurface::functions() const {
  return functions_;
}

bool InstanceDeviceSurface::ready() const {
  return ready_;
}

}  // namespace iggy3d::vulkan
