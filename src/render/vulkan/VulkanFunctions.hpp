#pragma once

#include "render/RenderDiagnostics.hpp"
#include "render/vulkan/VulkanTypes.hpp"

#if defined(IGGY3D_HAS_VULKAN)
#include <vulkan/vulkan.h>
#else
using PFN_vkVoidFunction = void (*)();
using PFN_vkGetPhysicalDeviceSurfaceSupportKHR = PFN_vkVoidFunction;
using PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR = PFN_vkVoidFunction;
using PFN_vkGetPhysicalDeviceSurfaceFormatsKHR = PFN_vkVoidFunction;
using PFN_vkGetPhysicalDeviceSurfacePresentModesKHR = PFN_vkVoidFunction;
using PFN_vkDestroySurfaceKHR = PFN_vkVoidFunction;
using PFN_vkCreateDebugUtilsMessengerEXT = PFN_vkVoidFunction;
using PFN_vkDestroyDebugUtilsMessengerEXT = PFN_vkVoidFunction;
using PFN_vkCreateSwapchainKHR = PFN_vkVoidFunction;
using PFN_vkDestroySwapchainKHR = PFN_vkVoidFunction;
using PFN_vkGetSwapchainImagesKHR = PFN_vkVoidFunction;
using PFN_vkAcquireNextImageKHR = PFN_vkVoidFunction;
using PFN_vkQueuePresentKHR = PFN_vkVoidFunction;
using PFN_vkCmdBeginRendering = PFN_vkVoidFunction;
using PFN_vkCmdEndRendering = PFN_vkVoidFunction;
#endif

namespace iggy3d::vulkan {

enum class VulkanLoaderStrategy : std::uint8_t {
  LinkedLoader,
  ManualLoader,
};

struct VulkanFunctionLoadConfig {
  VulkanLoaderStrategy loaderStrategy = VulkanLoaderStrategy::LinkedLoader;
  bool debugUtilsEnabled = false;
  bool debugUtilsRequired = false;
  bool swapchainEnabled = true;
  bool dynamicRenderingCore13 = true;
  bool dynamicRenderingKhr = false;
};

struct VulkanInstanceFunctions {
  PFN_vkGetPhysicalDeviceSurfaceSupportKHR getPhysicalDeviceSurfaceSupportKHR = nullptr;
  PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR getPhysicalDeviceSurfaceCapabilitiesKHR = nullptr;
  PFN_vkGetPhysicalDeviceSurfaceFormatsKHR getPhysicalDeviceSurfaceFormatsKHR = nullptr;
  PFN_vkGetPhysicalDeviceSurfacePresentModesKHR getPhysicalDeviceSurfacePresentModesKHR = nullptr;
  PFN_vkDestroySurfaceKHR destroySurfaceKHR = nullptr;
  PFN_vkCreateDebugUtilsMessengerEXT createDebugUtilsMessengerEXT = nullptr;
  PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugUtilsMessengerEXT = nullptr;
};

struct VulkanDeviceFunctions {
  PFN_vkCreateSwapchainKHR createSwapchainKHR = nullptr;
  PFN_vkDestroySwapchainKHR destroySwapchainKHR = nullptr;
  PFN_vkGetSwapchainImagesKHR getSwapchainImagesKHR = nullptr;
  PFN_vkAcquireNextImageKHR acquireNextImageKHR = nullptr;
  PFN_vkQueuePresentKHR queuePresentKHR = nullptr;
  PFN_vkCmdBeginRendering cmdBeginRendering = nullptr;
  PFN_vkCmdEndRendering cmdEndRendering = nullptr;
};

struct VulkanFunctionTables {
  VulkanInstanceFunctions instance;
  VulkanDeviceFunctions device;
  RenderReceipt receipt;
  bool clean = false;
};

VulkanFunctionTables loadVulkanFunctions(VkInstance instance,
                                         VkDevice device,
                                         const VulkanFunctionLoadConfig& config);
std::string_view vulkanLoaderStrategyName(VulkanLoaderStrategy strategy);

}  // namespace iggy3d::vulkan
