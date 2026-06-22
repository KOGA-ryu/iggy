#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <vector>

#if defined(IGGY3D_HAS_VULKAN)
#include <vulkan/vulkan.h>
#endif

#if defined(IGGY3D_HAS_VULKAN) && !defined(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME)
#define VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME "VK_KHR_portability_subset"
#endif

#if !defined(IGGY3D_SDL3_SOURCE_VALUE)
#define IGGY3D_SDL3_SOURCE_VALUE "missing"
#endif

#if !defined(IGGY3D_VULKAN_LOADER_VALUE)
#define IGGY3D_VULKAN_LOADER_VALUE "missing"
#endif

#if !defined(IGGY3D_VULKAN_SDK_ROOT_VALUE)
#define IGGY3D_VULKAN_SDK_ROOT_VALUE ""
#endif

#if !defined(IGGY3D_VULKAN_SDK_SOURCE_VALUE)
#define IGGY3D_VULKAN_SDK_SOURCE_VALUE "not_found"
#endif

#if !defined(IGGY3D_VULKAN_ICD_PATH_VALUE)
#define IGGY3D_VULKAN_ICD_PATH_VALUE ""
#endif

#if !defined(IGGY3D_GLSLC_PATH_VALUE)
#define IGGY3D_GLSLC_PATH_VALUE ""
#endif

namespace {

bool strictLane() {
#if defined(IGGY3D_REQUIRE_VULKAN_SMOKE_ENABLED)
  return true;
#else
  return false;
#endif
}

bool sdl3Available() {
#if defined(IGGY3D_HAS_SDL3)
  return true;
#else
  return false;
#endif
}

bool vulkanAvailable() {
#if defined(IGGY3D_HAS_VULKAN)
  return true;
#else
  return false;
#endif
}

std::string_view platformName() {
#if defined(__APPLE__)
  return "macos";
#elif defined(__linux__)
  return "linux";
#elif defined(_WIN32)
  return "windows";
#else
  return "unknown";
#endif
}

std::string_view platformLane() {
#if defined(__APPLE__)
  return "moltenvk";
#elif defined(__linux__)
  return "native_vulkan";
#elif defined(_WIN32)
  return "native_vulkan";
#else
  return "unknown";
#endif
}

bool pathPresent(std::string_view value) {
  return !value.empty() && std::filesystem::exists(std::filesystem::path{value});
}

#if defined(IGGY3D_HAS_VULKAN)
bool instanceExtensionAvailable(const char* requested) {
  std::uint32_t count = 0;
  if (vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr) != VK_SUCCESS) {
    return false;
  }
  std::vector<VkExtensionProperties> extensions(count);
  if (count != 0U &&
      vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data()) != VK_SUCCESS) {
    return false;
  }
  for (const VkExtensionProperties& extension : extensions) {
    if (std::strcmp(extension.extensionName, requested) == 0) {
      return true;
    }
  }
  return false;
}

bool validationLayerAvailable() {
  std::uint32_t count = 0;
  if (vkEnumerateInstanceLayerProperties(&count, nullptr) != VK_SUCCESS) {
    return false;
  }
  std::vector<VkLayerProperties> layers(count);
  if (count != 0U && vkEnumerateInstanceLayerProperties(&count, layers.data()) != VK_SUCCESS) {
    return false;
  }
  for (const VkLayerProperties& layer : layers) {
    if (std::strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") == 0) {
      return true;
    }
  }
  return false;
}

struct DeviceProbe {
  std::uint32_t physicalDeviceCount{0};
  bool portabilitySubsetExposed{false};
};

DeviceProbe probeDevices(bool portabilityEnumerationAvailable) {
  DeviceProbe result;

  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "iggy3d macos dependency probe";
  appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
  appInfo.pEngineName = "iggy3d";
  appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
  appInfo.apiVersion = VK_API_VERSION_1_2;

  std::vector<const char*> instanceExtensions;
  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
#if defined(__APPLE__)
  if (portabilityEnumerationAvailable) {
    instanceExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
  }
#endif
  createInfo.enabledExtensionCount = static_cast<std::uint32_t>(instanceExtensions.size());
  createInfo.ppEnabledExtensionNames =
      instanceExtensions.empty() ? nullptr : instanceExtensions.data();

  VkInstance instance = VK_NULL_HANDLE;
  if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
    return result;
  }

  if (vkEnumeratePhysicalDevices(instance, &result.physicalDeviceCount, nullptr) != VK_SUCCESS ||
      result.physicalDeviceCount == 0U) {
    vkDestroyInstance(instance, nullptr);
    result.physicalDeviceCount = 0U;
    return result;
  }

  std::vector<VkPhysicalDevice> devices(result.physicalDeviceCount);
  if (vkEnumeratePhysicalDevices(instance, &result.physicalDeviceCount, devices.data()) !=
      VK_SUCCESS) {
    vkDestroyInstance(instance, nullptr);
    result.physicalDeviceCount = 0U;
    return result;
  }

  for (VkPhysicalDevice device : devices) {
    std::uint32_t extensionCount = 0;
    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr) !=
        VK_SUCCESS) {
      continue;
    }
    std::vector<VkExtensionProperties> extensions(extensionCount);
    if (extensionCount != 0U &&
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                             extensions.data()) != VK_SUCCESS) {
      continue;
    }
    for (const VkExtensionProperties& extension : extensions) {
      if (std::strcmp(extension.extensionName, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME) == 0) {
        result.portabilitySubsetExposed = true;
      }
    }
  }

  vkDestroyInstance(instance, nullptr);
  return result;
}
#endif

}  // namespace

int main() {
  const bool strict = strictLane();
  const bool sdl = sdl3Available();
  const bool vulkan = vulkanAvailable();
  const bool sdkFound = pathPresent(IGGY3D_VULKAN_SDK_ROOT_VALUE);
  const bool icdFound = pathPresent(IGGY3D_VULKAN_ICD_PATH_VALUE);
  const bool glslcFound = pathPresent(IGGY3D_GLSLC_PATH_VALUE);

  bool validationFound = false;
  bool portabilityEnumerationAvailable = false;
  std::uint32_t physicalDeviceCount = 0;
  bool portabilitySubsetExposed = false;
#if defined(IGGY3D_HAS_VULKAN)
  validationFound = validationLayerAvailable();
  portabilityEnumerationAvailable =
      instanceExtensionAvailable(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
  const DeviceProbe devices = probeDevices(portabilityEnumerationAvailable);
  physicalDeviceCount = devices.physicalDeviceCount;
  portabilitySubsetExposed = devices.portabilitySubsetExposed;
#endif

#if defined(__APPLE__)
  const bool portabilityRequired = vulkan;
  const bool moltenVkAvailable = vulkan && (icdFound || portabilitySubsetExposed);
#else
  const bool portabilityRequired = false;
  const bool moltenVkAvailable = false;
#endif

  std::string_view reason = "macos_dependency_probe_ok";
  bool ready = true;
  if (!sdl) {
    reason = "sdl3_unavailable";
    ready = false;
  } else if (!vulkan) {
    reason = "vulkan_loader_missing";
    ready = false;
#if defined(__APPLE__)
  } else if (!sdkFound) {
    reason = "vulkan_sdk_missing";
    ready = false;
  } else if (!icdFound) {
    reason = "vulkan_icd_missing";
    ready = false;
  } else if (!moltenVkAvailable) {
    reason = "moltenvk_unavailable";
    ready = false;
  } else if (!portabilityEnumerationAvailable) {
    reason = "portability_enumeration_missing";
    ready = false;
#endif
  }

  const std::string_view result = ready ? "pass" : (strict ? "fail" : "skip");

  std::cout << "smoke=macos_vulkan_dependency\n";
  std::cout << "platform=" << platformName() << "\n";
  std::cout << "platform_lane=" << platformLane() << "\n";
  std::cout << "sdl3_target_available=" << (sdl ? "true" : "false") << "\n";
  std::cout << "sdl3_source=" << IGGY3D_SDL3_SOURCE_VALUE << "\n";
  std::cout << "vulkan_loader_found=" << (vulkan ? "true" : "false") << "\n";
  std::cout << "vulkan_loader_source=" << IGGY3D_VULKAN_LOADER_VALUE << "\n";
  std::cout << "vulkan_sdk_root=" << IGGY3D_VULKAN_SDK_ROOT_VALUE << "\n";
  std::cout << "vulkan_sdk_source=" << IGGY3D_VULKAN_SDK_SOURCE_VALUE << "\n";
  std::cout << "vulkan_icd_path=" << IGGY3D_VULKAN_ICD_PATH_VALUE << "\n";
  std::cout << "vulkan_icd_found=" << (icdFound ? "true" : "false") << "\n";
  std::cout << "moltenvk_available=";
#if defined(__APPLE__)
  std::cout << (vulkan ? (moltenVkAvailable ? "true" : "false") : "unavailable") << "\n";
#else
  std::cout << "unavailable\n";
#endif
  std::cout << "glslc_path=" << IGGY3D_GLSLC_PATH_VALUE << "\n";
  std::cout << "glslc_found=" << (glslcFound ? "true" : "false") << "\n";
  std::cout << "validation_layer_found="
            << (vulkan ? (validationFound ? "true" : "false") : "unavailable") << "\n";
  std::cout << "sync_validation_available="
            << (vulkan ? (validationFound ? "true" : "false") : "unavailable") << "\n";
  std::cout << "portability_enumeration_available="
            << (vulkan ? (portabilityEnumerationAvailable ? "true" : "false") : "unavailable")
            << "\n";
  std::cout << "portability_enumeration_required="
            << (vulkan ? (portabilityRequired ? "true" : "false") : "unavailable") << "\n";
  std::cout << "portability_subset_exposed="
            << (vulkan ? (portabilitySubsetExposed ? "true" : "false") : "unavailable")
            << "\n";
  std::cout << "physical_device_count=" << physicalDeviceCount << "\n";
  std::cout << "strict_lane=" << (strict ? "true" : "false") << "\n";
  std::cout << "result=" << result << "\n";
  std::cout << "reason_code=" << reason << "\n";

  if (ready) {
    return 0;
  }
  return strict ? 1 : 77;
}
