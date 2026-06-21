#include <filesystem>
#include <iostream>
#include <string_view>
#include <vector>

#if defined(IGGY3D_HAS_VULKAN)
#include <cstring>
#include <vulkan/vulkan.h>
#endif

namespace {

std::string_view platformName() {
#if defined(__APPLE__)
  return "macos";
#elif defined(_WIN32)
  return "windows";
#else
  return "linux";
#endif
}

bool shaderArtifactsFound() {
#if defined(IGGY3D_SHADER_BINARY_ROOT_VALUE)
  const std::filesystem::path root{IGGY3D_SHADER_BINARY_ROOT_VALUE};
  return std::filesystem::exists(root / "first_room.vert.spv") &&
         std::filesystem::exists(root / "first_room.frag.spv");
#else
  return false;
#endif
}

std::filesystem::path shaderRoot() {
#if defined(IGGY3D_SHADER_BINARY_ROOT_VALUE)
  return std::filesystem::path{IGGY3D_SHADER_BINARY_ROOT_VALUE};
#else
  return {};
#endif
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

bool validationLayerFound() {
  std::uint32_t count = 0;
  if (vkEnumerateInstanceLayerProperties(&count, nullptr) != VK_SUCCESS) {
    return false;
  }
  std::vector<VkLayerProperties> layers(count);
  if (count != 0U &&
      vkEnumerateInstanceLayerProperties(&count, layers.data()) != VK_SUCCESS) {
    return false;
  }
  for (const VkLayerProperties& layer : layers) {
    if (std::strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") == 0) {
      return true;
    }
  }
  return false;
}

std::uint32_t physicalDeviceCount() {
  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "iggy3d package vulkan dependency smoke";
  appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
  appInfo.pEngineName = "iggy3d";
  appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
  appInfo.apiVersion = VK_API_VERSION_1_2;

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
#if defined(__APPLE__)
  std::vector<const char*> extensions;
  if (instanceExtensionAvailable(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
  }
  createInfo.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();
#endif
  VkInstance instance = VK_NULL_HANDLE;
  if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
    return 0;
  }
  std::uint32_t count = 0;
  if (vkEnumeratePhysicalDevices(instance, &count, nullptr) != VK_SUCCESS) {
    count = 0;
  }
  vkDestroyInstance(instance, nullptr);
  return count;
}
#endif

}  // namespace

int main() {
  std::uint32_t deviceCount = 0;
  bool validationFound = false;
#if defined(IGGY3D_HAS_VULKAN)
  deviceCount = physicalDeviceCount();
  validationFound = validationLayerFound();
#endif

  const bool sdlFound =
#if defined(IGGY3D_HAS_SDL3)
      true;
#else
      false;
#endif
  const bool vulkanFound =
#if defined(IGGY3D_HAS_VULKAN)
      true;
#else
      false;
#endif
  const bool shadersFound = shaderArtifactsFound();
  const bool pass = vulkanFound && sdlFound && deviceCount > 0U && shadersFound;
  const char* reason = "packet7_package_smoke_pass";
  if (!vulkanFound) {
    reason = "vulkan_smoke_skipped_loader_missing";
  } else if (!sdlFound) {
    reason = "vulkan_smoke_skipped_sdl_missing";
  } else if (deviceCount == 0U) {
    reason = "vulkan_smoke_skipped_no_suitable_device";
  } else if (!shadersFound) {
    reason = "shader_artifact_missing";
  }

  std::cout << "smoke=package_vulkan_dependency\n";
  std::cout << "platform=" << platformName() << "\n";
  std::cout << "package_mode=build_tree_visual\n";
  std::cout << "sdl_runtime_found=" << (sdlFound ? "true" : "false") << "\n";
  std::cout << "vulkan_loader_found=" << (vulkanFound ? "true" : "false") << "\n";
  std::cout << "physical_device_count=" << deviceCount << "\n";
  std::cout << "wsi_available=" << (sdlFound && vulkanFound ? "true" : "false") << "\n";
#if defined(__APPLE__)
  bool moltenVkAvailable = false;
#if defined(IGGY3D_HAS_VULKAN)
  moltenVkAvailable = instanceExtensionAvailable(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif
  std::cout << "moltenvk_available=" << (moltenVkAvailable ? "true" : "false") << "\n";
#else
  std::cout << "moltenvk_available=not_applicable\n";
#endif
  std::cout << "validation_layer_found=" << (validationFound ? "true" : "false") << "\n";
  std::cout << "sync_validation_available=" << (validationFound ? "true" : "false") << "\n";
  std::cout << "shader_root=" << shaderRoot().string() << "\n";
  std::cout << "shader_artifacts_found=" << (shadersFound ? "true" : "false") << "\n";
  std::cout << "result=" << (pass ? "pass" : "skip") << "\n";
  std::cout << "reason_code=" << reason << "\n";
  return pass ? 0 : 77;
}
