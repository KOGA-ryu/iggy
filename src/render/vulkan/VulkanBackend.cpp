#include "render/vulkan/VulkanBackend.hpp"

#include "render/vulkan/VulkanResult.hpp"

#include <filesystem>

namespace iggy3d {
namespace {

RenderReason backendReason(std::string_view code) {
  if (code == "vulkan_backend_no_swapchain_yet") {
    return {code, "vulkan backend has no swapchain yet"};
  }
  if (code == "render_loop_not_ready") {
    return {code, "render loop not ready"};
  }
  if (code == "empty_frame_presented") {
    return {code, "empty frame presented"};
  }
  if (code == "vulkan_backend_shutdown") {
    return {code, "vulkan backend shutdown"};
  }
  if (code == "swapchain_not_drawable") {
    return {code, "swapchain not drawable"};
  }
  if (code == "vulkan_smoke_pass") {
    return {code, "vulkan smoke pass"};
  }
  if (code == "packet7_first_room_visible") {
    return {code, "packet 7 first room visible"};
  }
  if (code == "shader_artifact_missing") {
    return {code, "shader artifact missing"};
  }
  return {"vulkan_surface_provider_missing", "vulkan surface provider missing"};
}

vulkan::DebugValidationConfig validationConfigFromRendererConfig(const RendererConfig& config) {
  vulkan::DebugValidationConfig validation;
  validation.validationMode = vulkan::validationRequestModeFromConfig(config.validation);
  validation.syncValidationMode = vulkan::validationRequestModeFromConfig(config.syncValidation);
  validation.strictVulkan = config.strictVulkan;
  validation.debugMessengerRequested = config.validation != ValidationMode::Off;
  validation.diagnosticsDir = config.diagnosticsDir;
  return validation;
}

}  // namespace

VulkanBackend::VulkanBackend(VulkanBackendCreateInfo createInfo)
    : config_(std::move(createInfo.config)) {
  vulkan::InstanceDeviceSurfaceCreateInfo bootstrapInfo;
  bootstrapInfo.config = config_;
  bootstrapInfo.surfaceProvider = std::move(createInfo.surfaceProvider);
  bootstrapInfo.validation = validationConfigFromRendererConfig(config_);
  bootstrapInfo.featureRequest.allowSoftwareDevice = config_.allowSoftwareVulkan;
  diagnostics_ = bootstrap_.initialize(bootstrapInfo);
  if (bootstrap_.ready()) {
    initializePacket5Modules(createInfo.drawableWidth, createInfo.drawableHeight);
  } else {
    lifecycleState_ = RendererLifecycleState::NotInitialized;
  }
}

VulkanBackend::~VulkanBackend() {
  shutdown();
}

RendererBackendKind VulkanBackend::backendKind() const {
  return RendererBackendKind::Vulkan;
}

RendererLifecycleState VulkanBackend::lifecycleState() const {
  return lifecycleState_;
}

RenderReceipt VulkanBackend::makeReceipt(std::string_view result,
                                         std::string_view reasonCode) const {
  RenderReceipt receipt;
  appendReceiptField(receipt, "receipt_version", "1");
  appendReceiptField(receipt, "repo", "iggy3d");
  appendReceiptField(receipt, "file_plan", "src/render/vulkan/VulkanBackend.cpp");
  appendReceiptField(receipt, "packet_order", "5");
  appendReceiptField(receipt, "allowed_to_implement_code_now", "true");
  appendReceiptField(receipt, "backend", "vulkan");
  appendReceiptField(receipt, "backend_phase", firstRoomReady_ ? "first_room_frame"
                                                               : "swapchain_empty_frame");
  appendReceiptField(receipt, "initialized", bootstrap_.ready());
  appendReceiptField(receipt, "device_ready", bootstrap_.ready());
  appendReceiptField(receipt, "surface_ready", bootstrap_.handles().surface != VkSurfaceKHR{});
  appendReceiptField(receipt, "swapchain_state", swapchainStateName(swapchain_.info().state));
  appendReceiptField(receipt, "swapchain_generation",
                     static_cast<std::uint64_t>(swapchain_.info().generation));
  appendReceiptField(receipt, "swapchain_recreate_count",
                     static_cast<std::uint64_t>(swapchain_.info().recreateCount));
  appendReceiptField(receipt, "sync_policy", "binary_wsi");
  appendReceiptField(receipt, "frame_slots", static_cast<std::uint64_t>(config_.maxFramesInFlight));
  appendReceiptField(receipt, "command_recording_ready", commandRecording_.ready());
  appendReceiptField(receipt, "render_loop_ready", renderLoop_.ready());
  appendReceiptField(receipt, "pipeline_family", "first_room");
  appendReceiptField(receipt, "pipeline_variant", vulkan::kFirstRoomPipelineVariant);
  appendReceiptField(receipt, "first_room_bundle_ready", firstRoomReady_);
  appendReceiptField(receipt, "pipeline_created", firstRoomPipeline_.pipeline != VkPipeline{});
  appendReceiptField(receipt, "creative_view_model_pipeline_created",
                     creativeViewModelPipeline_.pipeline != VkPipeline{});
  appendReceiptField(receipt, "material_texture_pipeline_created",
                     materialTexturePipeline_.pipeline != VkPipeline{});
  appendReceiptField(receipt, "vertex_buffer_count",
                     firstRoomResources_.ready() ? static_cast<std::uint64_t>(1)
                                                 : static_cast<std::uint64_t>(0));
  appendReceiptField(receipt, "index_buffer_count",
                     firstRoomResources_.ready() ? static_cast<std::uint64_t>(1)
                                                 : static_cast<std::uint64_t>(0));
  appendReceiptField(receipt, "depth_enabled", firstRoomResources_.ready());
  appendReceiptField(receipt, "screenshot_capture", frameCapture_.ready() ? "enabled" : "unavailable");
  appendReceiptField(receipt, "validation", "unavailable");
  appendReceiptField(receipt, "sync_validation", "unavailable");
  appendReceiptField(receipt, "function_loading_clean", bootstrap_.functions().clean);
  appendReceiptField(receipt, "last_resize_width", static_cast<std::uint64_t>(lastResize_.width));
  appendReceiptField(receipt, "last_resize_height", static_cast<std::uint64_t>(lastResize_.height));
  appendReceiptField(receipt, "result", result);
  appendReceiptField(receipt, "reason_code", reasonCode);
  return receipt;
}

bool VulkanBackend::initializeStaticMeshMaterialPipeline() {
  destroyStaticMeshMaterialPipeline();
  const VkDescriptorSetLayout textureLayout =
      firstRoomResources_.staticMeshMaterialTextures().descriptorSetLayout;
  if (textureLayout == VK_NULL_HANDLE) {
    return false;
  }

  vulkan::ShaderModuleCreateInfo vertexInfo;
  vertexInfo.device = bootstrap_.handles().device;
  vertexInfo.spirvPath =
      config_.shaderRoot / "material_unlit_textured.vert.spv";
  vertexInfo.stage = vulkan::ShaderStage::Vertex;
  vertexInfo.debugName = "material_unlit_textured.vertex";
  const vulkan::ShaderModuleResult vertexResult =
      vulkan::createShaderModule(vertexInfo);
  if (vertexResult.outcome != RenderOutcome::Ok) {
    return false;
  }
  materialTextureVertexShader_ = vertexResult.record;

  vulkan::ShaderModuleCreateInfo fragmentInfo;
  fragmentInfo.device = bootstrap_.handles().device;
  fragmentInfo.spirvPath =
      config_.shaderRoot / "material_unlit_textured.frag.spv";
  fragmentInfo.stage = vulkan::ShaderStage::Fragment;
  fragmentInfo.debugName = "material_unlit_textured.fragment";
  const vulkan::ShaderModuleResult fragmentResult =
      vulkan::createShaderModule(fragmentInfo);
  if (fragmentResult.outcome != RenderOutcome::Ok) {
    destroyStaticMeshMaterialPipeline();
    return false;
  }
  materialTextureFragmentShader_ = fragmentResult.record;

  vulkan::PipelineLayoutCreateInfo layoutInfo;
  layoutInfo.device = bootstrap_.handles().device;
  layoutInfo.key.layout = "material_texture";
  layoutInfo.key.descriptorSetLayoutCount =
      vulkan::kMaterialTextureDescriptorSetLayoutCount;
  layoutInfo.descriptorSetLayouts = &textureLayout;
  const vulkan::PipelineLayoutResult layoutResult =
      vulkan::createFirstRoomPipelineLayout(layoutInfo);
  if (layoutResult.outcome != RenderOutcome::Ok) {
    destroyStaticMeshMaterialPipeline();
    return false;
  }
  materialTextureLayout_ = layoutResult.record;

  vulkan::FirstRoomPipelineCreateInfo pipelineInfo;
  pipelineInfo.device = bootstrap_.handles().device;
  pipelineInfo.colorFormat = swapchain_.info().colorFormat;
  pipelineInfo.depthFormat = VK_FORMAT_D32_SFLOAT;
  pipelineInfo.vertexShader = materialTextureVertexShader_;
  pipelineInfo.fragmentShader = materialTextureFragmentShader_;
  pipelineInfo.layout = materialTextureLayout_;
  pipelineInfo.flavor = vulkan::FirstRoomPipelineFlavor::MaterialTextured;
  const vulkan::FirstRoomPipelineResult pipelineResult =
      vulkan::createFirstRoomPipeline(pipelineInfo);
  if (pipelineResult.outcome != RenderOutcome::Ok) {
    destroyStaticMeshMaterialPipeline();
    return false;
  }
  materialTexturePipeline_ = pipelineResult.record;
  return true;
}

void VulkanBackend::destroyStaticMeshMaterialPipeline() {
  vulkan::destroyFirstRoomPipeline(bootstrap_.handles().device,
                                   materialTexturePipeline_);
  vulkan::destroyPipelineLayout(bootstrap_.handles().device,
                                materialTextureLayout_);
  vulkan::destroyShaderModule(bootstrap_.handles().device,
                              materialTextureFragmentShader_);
  vulkan::destroyShaderModule(bootstrap_.handles().device,
                              materialTextureVertexShader_);
}

void VulkanBackend::initializePacket7FirstRoomModules() {
  destroyPacket7FirstRoomModules();
  firstRoomReady_ = false;
  if (config_.shaderRoot.empty()) {
    diagnostics_ = makeReceipt("skip", "shader_artifact_missing");
    return;
  }

  const std::filesystem::path vertexPath = config_.shaderRoot / "first_room.vert.spv";
  const std::filesystem::path fragmentPath = config_.shaderRoot / "first_room.frag.spv";

  vulkan::ShaderModuleCreateInfo vertexInfo;
  vertexInfo.device = bootstrap_.handles().device;
  vertexInfo.spirvPath = vertexPath;
  vertexInfo.stage = vulkan::ShaderStage::Vertex;
  vertexInfo.debugName = "first_room.vertex";
  const vulkan::ShaderModuleResult vertexResult = vulkan::createShaderModule(vertexInfo);
  diagnostics_ = vertexResult.receipt;
  if (vertexResult.outcome != RenderOutcome::Ok) {
    return;
  }
  firstRoomVertexShader_ = vertexResult.record;

  vulkan::ShaderModuleCreateInfo fragmentInfo;
  fragmentInfo.device = bootstrap_.handles().device;
  fragmentInfo.spirvPath = fragmentPath;
  fragmentInfo.stage = vulkan::ShaderStage::Fragment;
  fragmentInfo.debugName = "first_room.fragment";
  const vulkan::ShaderModuleResult fragmentResult = vulkan::createShaderModule(fragmentInfo);
  diagnostics_ = fragmentResult.receipt;
  if (fragmentResult.outcome != RenderOutcome::Ok) {
    return;
  }
  firstRoomFragmentShader_ = fragmentResult.record;

  vulkan::PipelineLayoutCreateInfo layoutInfo;
  layoutInfo.device = bootstrap_.handles().device;
  const vulkan::PipelineLayoutResult layoutResult =
      vulkan::createFirstRoomPipelineLayout(layoutInfo);
  diagnostics_ = layoutResult.receipt;
  if (layoutResult.outcome != RenderOutcome::Ok) {
    return;
  }
  firstRoomLayout_ = layoutResult.record;

  vulkan::FirstRoomPipelineCreateInfo pipelineInfo;
  pipelineInfo.device = bootstrap_.handles().device;
  pipelineInfo.colorFormat = swapchain_.info().colorFormat;
  pipelineInfo.depthFormat = VK_FORMAT_D32_SFLOAT;
  pipelineInfo.vertexShader = firstRoomVertexShader_;
  pipelineInfo.fragmentShader = firstRoomFragmentShader_;
  pipelineInfo.layout = firstRoomLayout_;
  const vulkan::FirstRoomPipelineResult pipelineResult =
      vulkan::createFirstRoomPipeline(pipelineInfo);
  diagnostics_ = pipelineResult.receipt;
  if (pipelineResult.outcome != RenderOutcome::Ok) {
    return;
  }
  firstRoomPipeline_ = pipelineResult.record;

  pipelineInfo.depthMode = vulkan::FirstRoomDepthMode::Disabled;
  const vulkan::FirstRoomPipelineResult viewModelPipelineResult =
      vulkan::createFirstRoomPipeline(pipelineInfo);
  diagnostics_ = viewModelPipelineResult.receipt;
  if (viewModelPipelineResult.outcome != RenderOutcome::Ok) {
    return;
  }
  creativeViewModelPipeline_ = viewModelPipelineResult.record;

  vulkan::BufferImageResourcesCreateInfo resourcesInfo;
  resourcesInfo.physicalDevice = bootstrap_.handles().physicalDevice;
  resourcesInfo.device = bootstrap_.handles().device;
  resourcesInfo.graphicsQueue = bootstrap_.handles().graphicsQueue;
  resourcesInfo.graphicsQueueFamily = bootstrap_.queues().graphicsFamily;
  resourcesInfo.extent = swapchain_.info().extent;
  resourcesInfo.depthFormat = VK_FORMAT_D32_SFLOAT;
  resourcesInfo.staticMeshAssetRoot = config_.staticMeshAssetRoot;
  const vulkan::BufferImageResourcesResult resourcesResult =
      firstRoomResources_.createFirstRoomResources(resourcesInfo);
  diagnostics_ = resourcesResult.receipt;
  if (resourcesResult.outcome != RenderOutcome::Ok) {
    return;
  }

  static_cast<void>(initializeStaticMeshMaterialPipeline());

  if (swapchain_.info().transferSourceSupported) {
    const RenderReceipt captureReceipt = frameCapture_.create(
        {bootstrap_.handles().physicalDevice, bootstrap_.handles().device,
         swapchain_.info().extent, swapchain_.info().colorFormat});
    diagnostics_ = captureReceipt;
  }

  firstRoomReady_ = true;
  diagnostics_ = makeReceipt("pass", "packet7_first_room_visible");
}

void VulkanBackend::destroyPacket7FirstRoomModules() {
  firstRoomReady_ = false;
  frameCapture_.destroy();
  destroyStaticMeshMaterialPipeline();
  vulkan::destroyFirstRoomPipeline(bootstrap_.handles().device,
                                   creativeViewModelPipeline_);
  vulkan::destroyFirstRoomPipeline(bootstrap_.handles().device, firstRoomPipeline_);
  vulkan::destroyPipelineLayout(bootstrap_.handles().device, firstRoomLayout_);
  vulkan::destroyShaderModule(bootstrap_.handles().device, firstRoomFragmentShader_);
  vulkan::destroyShaderModule(bootstrap_.handles().device, firstRoomVertexShader_);
  firstRoomResources_.destroy();
}

void VulkanBackend::initializePacket5Modules(std::uint32_t drawableWidth,
                                             std::uint32_t drawableHeight) {
  vulkan::SwapchainCreateInfo swapchainInfo;
  swapchainInfo.device = bootstrap_.handles().device;
  swapchainInfo.physicalDevice = bootstrap_.handles().physicalDevice;
  swapchainInfo.surface = bootstrap_.handles().surface;
  swapchainInfo.queues = bootstrap_.queues();
  swapchainInfo.functions = bootstrap_.functions();
  swapchainInfo.drawableWidth = drawableWidth;
  swapchainInfo.drawableHeight = drawableHeight;
  swapchainInfo.config = config_;
  const vulkan::SwapchainOperationResult swapchainResult = swapchain_.create(swapchainInfo);
  diagnostics_ = swapchainResult.receipt;
  if (swapchainResult.outcome != RenderOutcome::Ok) {
    lifecycleState_ = RendererLifecycleState::NotInitialized;
    return;
  }

  vulkan::FrameSyncCreateInfo syncInfo;
  syncInfo.device = bootstrap_.handles().device;
  syncInfo.frameSlotCount = config_.maxFramesInFlight;
  const vulkan::FrameSyncOperationResult syncResult = frameSync_.create(syncInfo);
  diagnostics_ = syncResult.receipt;
  if (syncResult.outcome != RenderOutcome::Ok) {
    lifecycleState_ = RendererLifecycleState::NotInitialized;
    return;
  }

  vulkan::CommandRecordingCreateInfo commandInfo;
  commandInfo.device = bootstrap_.handles().device;
  commandInfo.deviceFunctions = bootstrap_.functions().device;
  commandInfo.graphicsQueueFamily = bootstrap_.queues().graphicsFamily;
  commandInfo.frameSlotCount = config_.maxFramesInFlight;
  const vulkan::CommandRecordResult commandResult = commandRecording_.create(commandInfo);
  diagnostics_ = commandResult.receipt;
  if (commandResult.outcome != RenderOutcome::Ok) {
    lifecycleState_ = RendererLifecycleState::NotInitialized;
    return;
  }

  initializePacket7FirstRoomModules();

  vulkan::RenderLoopCreateInfo loopInfo;
  loopInfo.deviceSurface = &bootstrap_;
  loopInfo.swapchain = &swapchain_;
  loopInfo.frameSync = &frameSync_;
  loopInfo.commandRecording = &commandRecording_;
  loopInfo.firstRoomPipeline = &firstRoomPipeline_;
  loopInfo.creativeViewModelPipeline = &creativeViewModelPipeline_;
  loopInfo.materialTexturePipeline = &materialTexturePipeline_;
  loopInfo.firstRoomLayout = &firstRoomLayout_;
  loopInfo.materialTextureLayout = &materialTextureLayout_;
  loopInfo.firstRoomResources = &firstRoomResources_;
  loopInfo.frameCapture = &frameCapture_;
  const vulkan::VulkanFrameResult loopResult = renderLoop_.initialize(loopInfo);
  diagnostics_ = loopResult.receipt;
  lifecycleState_ = loopResult.outcome == RenderOutcome::Ok ? RendererLifecycleState::Ready
                                                            : RendererLifecycleState::NotInitialized;
}

RenderSubmitResult VulkanBackend::submitFrame(const FrameInput& frame) {
  (void)frame;
  RenderSubmitResult result;
  if (lifecycleState_ == RendererLifecycleState::Shutdown) {
    result.outcome = RenderOutcome::RendererNotReady;
    result.reason = backendReason("vulkan_backend_shutdown");
    result.receipt = makeReceipt("fail", result.reason.code);
    diagnostics_ = result.receipt;
    return result;
  }
  if (lifecycleState_ != RendererLifecycleState::Ready || !renderLoop_.ready()) {
    result.outcome = RenderOutcome::SkipFrame;
    result.reason = backendReason("vulkan_backend_no_swapchain_yet");
    result.receipt = makeReceipt("skip", result.reason.code);
    diagnostics_ = result.receipt;
    return result;
  }
  const vulkan::VulkanFrameResult frameResult = renderLoop_.renderFrame(frame);
  result.outcome = frameResult.outcome;
  result.reason = frameResult.reason;
  result.receipt = frameResult.receipt;
  diagnostics_ = result.receipt;
  return result;
}

RenderSubmitResult VulkanBackend::resize(RenderViewport viewport) {
  lastResize_ = viewport;
  RenderSubmitResult result;
  if (lifecycleState_ == RendererLifecycleState::Shutdown) {
    result.outcome = RenderOutcome::RendererNotReady;
    result.reason = backendReason("vulkan_backend_shutdown");
    result.receipt = makeReceipt("fail", result.reason.code);
    diagnostics_ = result.receipt;
    return result;
  }
  if (!renderLoop_.ready() && swapchain_.info().state != vulkan::SwapchainState::NotDrawable) {
    result.outcome = RenderOutcome::SkipFrame;
    result.reason = backendReason("vulkan_backend_no_swapchain_yet");
    result.receipt = makeReceipt("skip", result.reason.code);
    diagnostics_ = result.receipt;
    return result;
  }
  if (!renderLoop_.ready() && swapchain_.info().state == vulkan::SwapchainState::NotDrawable) {
    if (viewport.width == 0U || viewport.height == 0U) {
      result.outcome = RenderOutcome::SkipFrame;
      result.reason = backendReason("swapchain_not_drawable");
      result.receipt = makeReceipt("skip", result.reason.code);
      diagnostics_ = result.receipt;
      return result;
    }
    if (bootstrap_.ready() && lifecycleState_ != RendererLifecycleState::Ready) {
      initializePacket5Modules(viewport.width, viewport.height);
      result.outcome = lifecycleState_ == RendererLifecycleState::Ready
                           ? RenderOutcome::Ok
                           : RenderOutcome::RendererNotReady;
      result.reason = lifecycleState_ == RendererLifecycleState::Ready
                          ? backendReason("vulkan_smoke_pass")
                          : backendReason("vulkan_backend_no_swapchain_yet");
      result.receipt = diagnostics_;
      return result;
    }
  }
  const vulkan::VulkanFrameResult resizeResult =
      renderLoop_.resize(viewport.width, viewport.height);
  if (resizeResult.outcome == RenderOutcome::Ok && swapchain_.ready()) {
    initializePacket7FirstRoomModules();
    vulkan::RenderLoopCreateInfo loopInfo;
    loopInfo.deviceSurface = &bootstrap_;
    loopInfo.swapchain = &swapchain_;
    loopInfo.frameSync = &frameSync_;
    loopInfo.commandRecording = &commandRecording_;
    loopInfo.firstRoomPipeline = &firstRoomPipeline_;
    loopInfo.creativeViewModelPipeline = &creativeViewModelPipeline_;
    loopInfo.materialTexturePipeline = &materialTexturePipeline_;
    loopInfo.firstRoomLayout = &firstRoomLayout_;
    loopInfo.materialTextureLayout = &materialTextureLayout_;
    loopInfo.firstRoomResources = &firstRoomResources_;
    loopInfo.frameCapture = &frameCapture_;
    renderLoop_.initialize(loopInfo);
  }
  result.outcome = resizeResult.outcome;
  result.reason = resizeResult.reason;
  result.receipt = resizeResult.receipt;
  diagnostics_ = result.receipt;
  return result;
}

RenderReceipt VulkanBackend::diagnostics() const {
  return diagnostics_;
}

bool VulkanBackend::frameCaptureReady() const {
  return frameCapture_.ready();
}

vulkan::NormalizedCapture VulkanBackend::readLastFrameCapture() const {
  return frameCapture_.readMappedRgba();
}

RenderOutcome VulkanBackend::waitIdle() {
#if defined(IGGY3D_HAS_VULKAN)
  if (bootstrap_.handles().device != VK_NULL_HANDLE) {
    const VkResult result = vkDeviceWaitIdle(bootstrap_.handles().device);
    if (result != VK_SUCCESS) {
      const vulkan::VulkanResultMapping mapped =
          vulkan::mapVkResult(result, vulkan::VulkanCallContext::QueueSubmit);
      return mapped.outcome;
    }
  }
#endif
  return RenderOutcome::Ok;
}

void VulkanBackend::shutdown() {
  if (lifecycleState_ == RendererLifecycleState::Shutdown) {
    return;
  }
  renderLoop_.shutdown();
  waitIdle();
  destroyPacket7FirstRoomModules();
  commandRecording_.destroy();
  frameSync_.destroy();
  swapchain_.destroy();
  bootstrap_.shutdown();
  lifecycleState_ = RendererLifecycleState::Shutdown;
  diagnostics_ = makeReceipt("pass", "vulkan_backend_shutdown");
}

}  // namespace iggy3d
