#include "render/vulkan/VulkanFunctions.hpp"

namespace iggy3d::vulkan {
namespace {

void appendBase(RenderReceipt& receipt) {
  appendReceiptField(receipt, "receipt_version", "1");
  appendReceiptField(receipt, "repo", "iggy3d");
  appendReceiptField(receipt, "file_plan", "src/render/vulkan/VulkanFunctions.cpp");
  appendReceiptField(receipt, "packet_order", "4");
  appendReceiptField(receipt, "allowed_to_implement_code_now", "false");
  appendReceiptField(receipt, "backend", "vulkan");
  appendReceiptField(receipt, "function_loading", "enabled");
}

#if defined(IGGY3D_HAS_VULKAN)
template <typename FunctionType>
FunctionType loadInstance(VkInstance instance, const char* name) {
  return reinterpret_cast<FunctionType>(vkGetInstanceProcAddr(instance, name));
}

template <typename FunctionType>
FunctionType loadDevice(VkDevice device, const char* name) {
  return reinterpret_cast<FunctionType>(vkGetDeviceProcAddr(device, name));
}
#endif

}  // namespace

std::string_view vulkanLoaderStrategyName(VulkanLoaderStrategy strategy) {
  switch (strategy) {
    case VulkanLoaderStrategy::LinkedLoader:
      return "linked_loader";
    case VulkanLoaderStrategy::ManualLoader:
      return "manual_loader";
  }
  return "linked_loader";
}

VulkanFunctionTables loadVulkanFunctions(VkInstance instance,
                                         VkDevice device,
                                         const VulkanFunctionLoadConfig& config) {
  VulkanFunctionTables tables;
  appendBase(tables.receipt);
  appendReceiptField(tables.receipt, "loader_strategy",
                     vulkanLoaderStrategyName(config.loaderStrategy));

#if defined(IGGY3D_HAS_VULKAN)
  if (instance != VK_NULL_HANDLE) {
    tables.instance.getPhysicalDeviceSurfaceSupportKHR =
        loadInstance<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(
            instance, "vkGetPhysicalDeviceSurfaceSupportKHR");
    tables.instance.getPhysicalDeviceSurfaceCapabilitiesKHR =
        loadInstance<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(
            instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    tables.instance.getPhysicalDeviceSurfaceFormatsKHR =
        loadInstance<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(
            instance, "vkGetPhysicalDeviceSurfaceFormatsKHR");
    tables.instance.getPhysicalDeviceSurfacePresentModesKHR =
        loadInstance<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(
            instance, "vkGetPhysicalDeviceSurfacePresentModesKHR");
    tables.instance.destroySurfaceKHR =
        loadInstance<PFN_vkDestroySurfaceKHR>(instance, "vkDestroySurfaceKHR");
    if (config.debugUtilsEnabled) {
      tables.instance.createDebugUtilsMessengerEXT =
          loadInstance<PFN_vkCreateDebugUtilsMessengerEXT>(instance,
                                                           "vkCreateDebugUtilsMessengerEXT");
      tables.instance.destroyDebugUtilsMessengerEXT =
          loadInstance<PFN_vkDestroyDebugUtilsMessengerEXT>(instance,
                                                            "vkDestroyDebugUtilsMessengerEXT");
    }
  }
  if (device != VK_NULL_HANDLE) {
    if (config.swapchainEnabled) {
      tables.device.createSwapchainKHR =
          loadDevice<PFN_vkCreateSwapchainKHR>(device, "vkCreateSwapchainKHR");
      tables.device.destroySwapchainKHR =
          loadDevice<PFN_vkDestroySwapchainKHR>(device, "vkDestroySwapchainKHR");
      tables.device.getSwapchainImagesKHR =
          loadDevice<PFN_vkGetSwapchainImagesKHR>(device, "vkGetSwapchainImagesKHR");
      tables.device.acquireNextImageKHR =
          loadDevice<PFN_vkAcquireNextImageKHR>(device, "vkAcquireNextImageKHR");
      tables.device.queuePresentKHR =
          loadDevice<PFN_vkQueuePresentKHR>(device, "vkQueuePresentKHR");
    }
    if (config.dynamicRenderingCore13) {
      tables.device.cmdBeginRendering =
          loadDevice<PFN_vkCmdBeginRendering>(device, "vkCmdBeginRendering");
      tables.device.cmdEndRendering =
          loadDevice<PFN_vkCmdEndRendering>(device, "vkCmdEndRendering");
    } else if (config.dynamicRenderingKhr) {
      tables.device.cmdBeginRendering =
          loadDevice<PFN_vkCmdBeginRendering>(device, "vkCmdBeginRenderingKHR");
      tables.device.cmdEndRendering =
          loadDevice<PFN_vkCmdEndRendering>(device, "vkCmdEndRenderingKHR");
    }
  }
#endif

  const bool surfaceQueryFunctionsLoaded =
      !config.swapchainEnabled ||
      (tables.instance.getPhysicalDeviceSurfaceSupportKHR != nullptr &&
       tables.instance.getPhysicalDeviceSurfaceCapabilitiesKHR != nullptr &&
       tables.instance.getPhysicalDeviceSurfaceFormatsKHR != nullptr &&
       tables.instance.getPhysicalDeviceSurfacePresentModesKHR != nullptr);
  const bool instanceLoaded =
      instance == VkInstance{} ||
      (tables.instance.destroySurfaceKHR != nullptr && surfaceQueryFunctionsLoaded);
  const bool debugLoaded =
      !config.debugUtilsEnabled ||
      (tables.instance.createDebugUtilsMessengerEXT != nullptr &&
       tables.instance.destroyDebugUtilsMessengerEXT != nullptr) ||
      !config.debugUtilsRequired;
  const bool swapchainLoaded =
      !config.swapchainEnabled ||
      (tables.device.createSwapchainKHR != nullptr &&
       tables.device.destroySwapchainKHR != nullptr &&
       tables.device.getSwapchainImagesKHR != nullptr &&
       tables.device.acquireNextImageKHR != nullptr &&
       tables.device.queuePresentKHR != nullptr);
  const bool dynamicLoaded =
      (!config.dynamicRenderingCore13 && !config.dynamicRenderingKhr) ||
      (tables.device.cmdBeginRendering != nullptr && tables.device.cmdEndRendering != nullptr);
  tables.clean = instanceLoaded && debugLoaded && swapchainLoaded && dynamicLoaded;

  appendReceiptField(tables.receipt, "instance_functions_loaded", instanceLoaded);
  appendReceiptField(tables.receipt, "device_functions_loaded",
                     device == VkDevice{} || (dynamicLoaded && swapchainLoaded));
  appendReceiptField(tables.receipt, "surface_query_functions_loaded",
                     surfaceQueryFunctionsLoaded);
  appendReceiptField(tables.receipt, "debug_utils_functions_loaded",
                     config.debugUtilsEnabled ? (debugLoaded ? "true" : "false")
                                              : "not_requested");
  appendReceiptField(tables.receipt, "swapchain_functions_loaded", swapchainLoaded);
  appendReceiptField(tables.receipt, "swapchain_image_query_function_loaded",
                     !config.swapchainEnabled ||
                         tables.device.getSwapchainImagesKHR != nullptr);
  appendReceiptField(tables.receipt, "swapchain_acquire_function_loaded",
                     !config.swapchainEnabled ||
                         tables.device.acquireNextImageKHR != nullptr);
  appendReceiptField(tables.receipt, "swapchain_present_function_loaded",
                     !config.swapchainEnabled ||
                         tables.device.queuePresentKHR != nullptr);
  appendReceiptField(tables.receipt, "dynamic_rendering_source",
                     config.dynamicRenderingCore13
                         ? "core_1_3"
                         : (config.dynamicRenderingKhr ? "khr_extension" : "unavailable"));
  appendReceiptField(tables.receipt, "begin_rendering_function",
                     tables.device.cmdBeginRendering == nullptr
                         ? "unavailable"
                         : (config.dynamicRenderingCore13 ? "vkCmdBeginRendering"
                                                          : "vkCmdBeginRenderingKHR"));
  appendReceiptField(tables.receipt, "end_rendering_function",
                     tables.device.cmdEndRendering == nullptr
                         ? "unavailable"
                         : (config.dynamicRenderingCore13 ? "vkCmdEndRendering"
                                                          : "vkCmdEndRenderingKHR"));
  appendReceiptField(tables.receipt, "core_khr_name_mix_detected",
                     config.dynamicRenderingCore13 && config.dynamicRenderingKhr);
  appendReceiptField(tables.receipt, "function_loading_clean", tables.clean);
  appendReceiptField(tables.receipt, "result", tables.clean ? "pass" : "fail");
  appendReceiptField(tables.receipt, "reason_code",
                     tables.clean ? "vulkan_smoke_pass" : "device_function_missing");
  return tables;
}

}  // namespace iggy3d::vulkan
