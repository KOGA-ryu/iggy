#pragma once

#include <cstdint>

#include "render/RenderBackend.hpp"
#include "render/vulkan/BufferImageResources.hpp"
#include "render/vulkan/CommandRecording.hpp"
#include "render/vulkan/FirstRoomPipeline.hpp"
#include "render/vulkan/FrameCapture.hpp"
#include "render/vulkan/FrameSync.hpp"
#include "render/vulkan/InstanceDeviceSurface.hpp"
#include "render/vulkan/PipelineLayout.hpp"
#include "render/vulkan/RenderLoop.hpp"
#include "render/vulkan/ShaderModule.hpp"
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
  bool frameCaptureReady() const;
  vulkan::NormalizedCapture readLastFrameCapture() const;

private:
  RenderReceipt makeReceipt(std::string_view result, std::string_view reasonCode) const;
  void initializePacket5Modules(std::uint32_t drawableWidth, std::uint32_t drawableHeight);
  void initializePacket7FirstRoomModules();
  void destroyPacket7FirstRoomModules();

  RendererConfig config_;
  vulkan::InstanceDeviceSurface bootstrap_;
  vulkan::Swapchain swapchain_;
  vulkan::FrameSync frameSync_;
  vulkan::CommandRecording commandRecording_;
  vulkan::RenderLoop renderLoop_;
  vulkan::ShaderModuleRecord firstRoomVertexShader_;
  vulkan::ShaderModuleRecord firstRoomFragmentShader_;
  vulkan::PipelineLayoutRecord firstRoomLayout_;
  vulkan::FirstRoomPipelineRecord firstRoomPipeline_;
  vulkan::BufferImageResources firstRoomResources_;
  vulkan::FrameCapture frameCapture_;
  bool firstRoomReady_ = false;
  RendererLifecycleState lifecycleState_ = RendererLifecycleState::NotInitialized;
  RenderReceipt diagnostics_;
  RenderViewport lastResize_;
};

}  // namespace iggy3d
