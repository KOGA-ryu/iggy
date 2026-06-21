#pragma once

#include "render/RenderBackend.hpp"
#include "render/vulkan/InstanceDeviceSurface.hpp"

namespace iggy3d {

struct VulkanBackendCreateInfo {
  RendererConfig config;
  vulkan::VulkanSurfaceProvider surfaceProvider;
};

class VulkanBackend final : public RenderBackend {
public:
  explicit VulkanBackend(VulkanBackendCreateInfo createInfo);
  ~VulkanBackend() override;

  VulkanBackend(const VulkanBackend&) = delete;
  VulkanBackend& operator=(const VulkanBackend&) = delete;

  RendererBackendKind backendKind() const override;
  RendererLifecycleState lifecycleState() const override;
  RenderSubmitResult submitFrame(const FrameInput& frame) override;
  RenderSubmitResult resize(RenderViewport viewport) override;
  RenderReceipt diagnostics() const override;
  RenderOutcome waitIdle() override;
  void shutdown() override;

private:
  RenderReceipt makeReceipt(std::string_view result, std::string_view reasonCode) const;

  RendererConfig config_;
  vulkan::InstanceDeviceSurface bootstrap_;
  RendererLifecycleState lifecycleState_ = RendererLifecycleState::NotInitialized;
  RenderReceipt diagnostics_;
  RenderViewport lastResize_;
};

}  // namespace iggy3d
