#pragma once

#include <cstdint>

#include "render/RenderBackend.hpp"
#include "render/vulkan/BufferImageResources.hpp"
#include "render/vulkan/CommandRecording.hpp"
#include "render/vulkan/DearImGuiVulkanBridge.hpp"
#include "render/vulkan/FirstRoomPipeline.hpp"
#include "render/vulkan/FrameCapture.hpp"
#include "render/vulkan/FrameSync.hpp"
#include "render/vulkan/InstanceDeviceSurface.hpp"
#include "render/vulkan/PipelineLayout.hpp"
#include "render/vulkan/RenderLoop.hpp"
#include "render/vulkan/ShaderModule.hpp"
#include "render/vulkan/Swapchain.hpp"

union SDL_Event;

namespace iggy3d {

struct VulkanBackendCreateInfo {
  RendererConfig config;
  vulkan::VulkanSurfaceProvider surfaceProvider;
  std::uint32_t drawableWidth = 640;
  std::uint32_t drawableHeight = 360;
  // Desktop UI shell (Dear ImGui). nativeWindow is the SDL_Window*; the shell
  // is only ever constructed when enableExternalUi is true — the --capture
  // construction gate (plan DL-1). Defaults keep every existing caller (and
  // all smokes) shell-free.
  void* nativeWindow = nullptr;
  bool enableExternalUi = false;
};

struct VulkanStaticMeshAssetReloadResult {
  RenderOutcome outcome = RenderOutcome::RendererNotReady;
  RenderReason reason{"static_mesh_asset_reload_not_requested",
                      "static mesh asset reload not requested"};
  RenderReceipt receipt;

  [[nodiscard]] bool ok() const noexcept {
    return outcome == RenderOutcome::Ok;
  }
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
  [[nodiscard]] VulkanStaticMeshAssetReloadResult reloadStaticMeshAssets();
  void shutdown() override;
  bool frameCaptureReady() const;
  vulkan::NormalizedCapture readLastFrameCapture() const;
  // Desktop UI shell surface (no-ops when the shell is disabled/unbuilt).
  void forwardExternalUiEvent(const SDL_Event& event);
  bool beginExternalUiFrame();
  bool externalUiFrameActive() const;
  bool externalUiRecordedLastFrame() const;
  bool externalUiWantsMouse() const;
  bool externalUiWantsKeyboard() const;

private:
  struct StaticMeshMaterialPipelineBundle {
    vulkan::ShaderModuleRecord vertexShader;
    vulkan::ShaderModuleRecord instanceVertexShader;
    vulkan::ShaderModuleRecord fragmentShader;
    vulkan::PipelineLayoutRecord layout;
    vulkan::FirstRoomPipelineRecord pipeline;
    vulkan::FirstRoomPipelineRecord instancePipeline;
  };

  RenderReceipt makeReceipt(std::string_view result, std::string_view reasonCode) const;
  void initializePacket5Modules(std::uint32_t drawableWidth, std::uint32_t drawableHeight);
  void initializePacket7FirstRoomModules();
  void destroyPacket7FirstRoomModules();
  bool initializeStaticMeshMaterialPipeline();
  bool createStaticMeshMaterialPipeline(
      VkDescriptorSetLayout textureLayout,
      StaticMeshMaterialPipelineBundle& output);
  void destroyStaticMeshMaterialPipelineBundle(
      StaticMeshMaterialPipelineBundle& bundle);
  void destroyStaticMeshMaterialPipeline();

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
  vulkan::FirstRoomPipelineRecord creativeViewModelPipeline_;
  vulkan::ShaderModuleRecord staticMeshInstanceVertexShader_;
  vulkan::FirstRoomPipelineRecord staticMeshInstancePipeline_;
  vulkan::ShaderModuleRecord materialTextureVertexShader_;
  vulkan::ShaderModuleRecord materialTextureInstanceVertexShader_;
  vulkan::ShaderModuleRecord materialTextureFragmentShader_;
  vulkan::PipelineLayoutRecord materialTextureLayout_;
  vulkan::FirstRoomPipelineRecord materialTexturePipeline_;
  vulkan::FirstRoomPipelineRecord materialTextureInstancePipeline_;
  vulkan::BufferImageResources firstRoomResources_;
  vulkan::FrameCapture frameCapture_;
  vulkan::DearImGuiVulkanBridge externalUiBridge_;
  void* externalUiNativeWindow_ = nullptr;
  bool externalUiEnabled_ = false;
  bool firstRoomReady_ = false;
  RendererLifecycleState lifecycleState_ = RendererLifecycleState::NotInitialized;
  RenderReceipt diagnostics_;
  RenderViewport lastResize_;
};

}  // namespace iggy3d
