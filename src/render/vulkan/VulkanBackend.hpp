#pragma once

#include <cstdint>

#include "render/RenderBackend.hpp"
#include "render/vulkan/CommandRecording.hpp"
#include "render/vulkan/FrameSync.hpp"
#include "render/vulkan/InstanceDeviceSurface.hpp"
#include "render/vulkan/RenderLoop.hpp"
#include "render/vulkan/Swapchain.hpp"

namespace iggy3d {

struct VulkanBackendCreateInfo {
  RendererConfig config;
  vulkan::VulkanSurfaceProvider surfaceProvider;
  std::uint32_t drawableWidth = 640;
  std::uint32_t drawableHeight = 360;
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
  void initializePacket5Modules(std::uint32_t drawableWidth, std::uint32_t drawableHeight);

  RendererConfig config_;
  vulkan::InstanceDeviceSurface bootstrap_;
  vulkan::Swapchain swapchain_;
  vulkan::FrameSync frameSync_;
  vulkan::CommandRecording commandRecording_;
  vulkan::RenderLoop renderLoop_;
  RendererLifecycleState lifecycleState_ = RendererLifecycleState::NotInitialized;
  RenderReceipt diagnostics_;
  RenderViewport lastResize_;
};

}  // namespace iggy3d
