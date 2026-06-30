#pragma once

#include <functional>
#include <string>
#include <vector>

#include "render/RendererConfig.hpp"
#include "render/vulkan/DebugValidation.hpp"
#include "render/vulkan/VulkanFeatureSupport.hpp"
#include "render/vulkan/VulkanFunctions.hpp"

namespace iggy3d::vulkan {

struct VulkanSurfaceProvider {
  std::vector<std::string> requiredInstanceExtensions;
  std::function<RenderReceipt(VkInstance, VkSurfaceKHR*)> createSurface;
};

struct InstanceDeviceSurfaceCreateInfo {
  RendererConfig config;
  VulkanSurfaceProvider surfaceProvider;
  DebugValidationConfig validation;
  VulkanFeatureBaselineRequest featureRequest;
};

class InstanceDeviceSurface {
public:
  InstanceDeviceSurface() = default;
  ~InstanceDeviceSurface();

  InstanceDeviceSurface(const InstanceDeviceSurface&) = delete;
  InstanceDeviceSurface& operator=(const InstanceDeviceSurface&) = delete;

  RenderReceipt initialize(const InstanceDeviceSurfaceCreateInfo& createInfo);
  RenderReceipt shutdown();

  const VulkanBootstrapHandles& handles() const;
  const VulkanQueueFamilySelection& queues() const;
  const VulkanFunctionTables& functions() const;
  bool ready() const;

private:
  void releaseHandles();

  VulkanBootstrapHandles handles_;
  VulkanQueueFamilySelection queues_;
  VulkanDeviceIdentity selectedDevice_;
  VulkanFunctionTables functions_;
  DebugValidation validation_;
  RenderReceipt lastReceipt_;
  bool ready_ = false;
};

}  // namespace iggy3d::vulkan
