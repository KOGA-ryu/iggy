#include "render/vulkan/VulkanBackend.hpp"

#include "render/vulkan/VulkanResult.hpp"

#include <filesystem>
#include <utility>

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
    : config_(std::move(createInfo.config)),
      externalUiNativeWindow_(createInfo.nativeWindow),
      externalUiEnabled_(createInfo.enableExternalUi) {
  if (!isValidRendererConfig(config_)) {
    diagnostics_ =
        makeReceipt("fail", "renderer_config_frames_in_flight_invalid");
    return;
  }

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
  appendReceiptField(receipt, "static_mesh_instance_pipeline_created",
                     staticMeshInstancePipeline_.pipeline != VkPipeline{});
  appendReceiptField(
      receipt, "static_mesh_instance_material_pipeline_created",
      materialTextureInstancePipeline_.pipeline != VkPipeline{});
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
  const VkDescriptorSetLayout textureLayout =
      firstRoomResources_.staticMeshMaterialTextures().descriptorSetLayout;
  if (textureLayout == VK_NULL_HANDLE) {
    return false;
  }

  StaticMeshMaterialPipelineBundle replacement;
  if (!createStaticMeshMaterialPipeline(textureLayout, replacement)) {
    return false;
  }
  destroyStaticMeshMaterialPipeline();
  materialTextureVertexShader_ = std::move(replacement.vertexShader);
  materialTextureInstanceVertexShader_ =
      std::move(replacement.instanceVertexShader);
  materialTextureFragmentShader_ = std::move(replacement.fragmentShader);
  materialTextureLayout_ = std::move(replacement.layout);
  materialTexturePipeline_ = std::move(replacement.pipeline);
  materialTextureInstancePipeline_ =
      std::move(replacement.instancePipeline);
  return true;
}

bool VulkanBackend::createStaticMeshMaterialPipeline(
    VkDescriptorSetLayout textureLayout,
    StaticMeshMaterialPipelineBundle& output) {
  output = {};
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
  output.vertexShader = vertexResult.record;

  vertexInfo.spirvPath =
      config_.shaderRoot / "static_mesh_instanced_textured.vert.spv";
  vertexInfo.debugName = "static_mesh_instanced_textured.vertex";
  const vulkan::ShaderModuleResult instanceVertexResult =
      vulkan::createShaderModule(vertexInfo);
  if (instanceVertexResult.outcome != RenderOutcome::Ok) {
    destroyStaticMeshMaterialPipelineBundle(output);
    return false;
  }
  output.instanceVertexShader = instanceVertexResult.record;

  vulkan::ShaderModuleCreateInfo fragmentInfo;
  fragmentInfo.device = bootstrap_.handles().device;
  fragmentInfo.spirvPath =
      config_.shaderRoot / "material_unlit_textured.frag.spv";
  fragmentInfo.stage = vulkan::ShaderStage::Fragment;
  fragmentInfo.debugName = "material_unlit_textured.fragment";
  const vulkan::ShaderModuleResult fragmentResult =
      vulkan::createShaderModule(fragmentInfo);
  if (fragmentResult.outcome != RenderOutcome::Ok) {
    destroyStaticMeshMaterialPipelineBundle(output);
    return false;
  }
  output.fragmentShader = fragmentResult.record;

  vulkan::PipelineLayoutCreateInfo layoutInfo;
  layoutInfo.device = bootstrap_.handles().device;
  layoutInfo.key.layout = "material_texture";
  layoutInfo.key.descriptorSetLayoutCount =
      vulkan::kMaterialTextureDescriptorSetLayoutCount;
  layoutInfo.descriptorSetLayouts = &textureLayout;
  const vulkan::PipelineLayoutResult layoutResult =
      vulkan::createFirstRoomPipelineLayout(layoutInfo);
  if (layoutResult.outcome != RenderOutcome::Ok) {
    destroyStaticMeshMaterialPipelineBundle(output);
    return false;
  }
  output.layout = layoutResult.record;

  vulkan::FirstRoomPipelineCreateInfo pipelineInfo;
  pipelineInfo.device = bootstrap_.handles().device;
  pipelineInfo.colorFormat = swapchain_.info().colorFormat;
  pipelineInfo.depthFormat = VK_FORMAT_D32_SFLOAT;
  pipelineInfo.vertexShader = output.vertexShader;
  pipelineInfo.fragmentShader = output.fragmentShader;
  pipelineInfo.layout = output.layout;
  pipelineInfo.flavor = vulkan::FirstRoomPipelineFlavor::MaterialTextured;
  const vulkan::FirstRoomPipelineResult pipelineResult =
      vulkan::createFirstRoomPipeline(pipelineInfo);
  if (pipelineResult.outcome != RenderOutcome::Ok) {
    destroyStaticMeshMaterialPipelineBundle(output);
    return false;
  }
  output.pipeline = pipelineResult.record;
  pipelineInfo.vertexShader = output.instanceVertexShader;
  pipelineInfo.instanced = true;
  const vulkan::FirstRoomPipelineResult instancePipelineResult =
      vulkan::createFirstRoomPipeline(pipelineInfo);
  if (instancePipelineResult.outcome != RenderOutcome::Ok) {
    destroyStaticMeshMaterialPipelineBundle(output);
    return false;
  }
  output.instancePipeline = instancePipelineResult.record;
  return true;
}

void VulkanBackend::destroyStaticMeshMaterialPipelineBundle(
    StaticMeshMaterialPipelineBundle& bundle) {
  vulkan::destroyFirstRoomPipeline(bootstrap_.handles().device,
                                   bundle.pipeline);
  vulkan::destroyFirstRoomPipeline(bootstrap_.handles().device,
                                   bundle.instancePipeline);
  vulkan::destroyPipelineLayout(bootstrap_.handles().device, bundle.layout);
  vulkan::destroyShaderModule(bootstrap_.handles().device,
                              bundle.fragmentShader);
  vulkan::destroyShaderModule(bootstrap_.handles().device,
                              bundle.vertexShader);
  vulkan::destroyShaderModule(bootstrap_.handles().device,
                              bundle.instanceVertexShader);
}

void VulkanBackend::destroyStaticMeshMaterialPipeline() {
  vulkan::destroyFirstRoomPipeline(bootstrap_.handles().device,
                                   materialTexturePipeline_);
  vulkan::destroyFirstRoomPipeline(bootstrap_.handles().device,
                                   materialTextureInstancePipeline_);
  vulkan::destroyPipelineLayout(bootstrap_.handles().device,
                                materialTextureLayout_);
  vulkan::destroyShaderModule(bootstrap_.handles().device,
                              materialTextureFragmentShader_);
  vulkan::destroyShaderModule(bootstrap_.handles().device,
                              materialTextureVertexShader_);
  vulkan::destroyShaderModule(bootstrap_.handles().device,
                              materialTextureInstanceVertexShader_);
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

  vertexInfo.spirvPath =
      config_.shaderRoot / "static_mesh_instanced.vert.spv";
  vertexInfo.debugName = "static_mesh_instanced.vertex";
  const vulkan::ShaderModuleResult instanceVertexResult =
      vulkan::createShaderModule(vertexInfo);
  diagnostics_ = instanceVertexResult.receipt;
  if (instanceVertexResult.outcome != RenderOutcome::Ok) {
    return;
  }
  staticMeshInstanceVertexShader_ = instanceVertexResult.record;

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

  pipelineInfo.vertexShader = staticMeshInstanceVertexShader_;
  pipelineInfo.instanced = true;
  const vulkan::FirstRoomPipelineResult instancePipelineResult =
      vulkan::createFirstRoomPipeline(pipelineInfo);
  diagnostics_ = instancePipelineResult.receipt;
  if (instancePipelineResult.outcome != RenderOutcome::Ok) {
    return;
  }
  staticMeshInstancePipeline_ = instancePipelineResult.record;

  pipelineInfo.vertexShader = firstRoomVertexShader_;
  pipelineInfo.instanced = false;
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
                                   staticMeshInstancePipeline_);
  vulkan::destroyFirstRoomPipeline(bootstrap_.handles().device,
                                   creativeViewModelPipeline_);
  vulkan::destroyFirstRoomPipeline(bootstrap_.handles().device, firstRoomPipeline_);
  vulkan::destroyPipelineLayout(bootstrap_.handles().device, firstRoomLayout_);
  vulkan::destroyShaderModule(bootstrap_.handles().device, firstRoomFragmentShader_);
  vulkan::destroyShaderModule(bootstrap_.handles().device,
                              staticMeshInstanceVertexShader_);
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

  vulkan::DearImGuiVulkanBridgeCreateInfo bridgeInfo;
  bridgeInfo.enabled = externalUiEnabled_;
  bridgeInfo.nativeWindow = externalUiNativeWindow_;
  bridgeInfo.instance = bootstrap_.handles().instance;
  bridgeInfo.physicalDevice = bootstrap_.handles().physicalDevice;
  bridgeInfo.device = bootstrap_.handles().device;
  bridgeInfo.graphicsQueueFamily = bootstrap_.queues().graphicsFamily;
  bridgeInfo.graphicsQueue = bootstrap_.handles().graphicsQueue;
#if defined(IGGY3D_HAS_VULKAN)
  bridgeInfo.apiVersion = VK_API_VERSION_1_3;
#endif
  bridgeInfo.deviceFunctions = bootstrap_.functions().device;
  bridgeInfo.swapchain = &swapchain_;
  static_cast<void>(externalUiBridge_.initialize(bridgeInfo));

  vulkan::RenderLoopCreateInfo loopInfo;
  loopInfo.deviceSurface = &bootstrap_;
  loopInfo.swapchain = &swapchain_;
  loopInfo.frameSync = &frameSync_;
  loopInfo.commandRecording = &commandRecording_;
  loopInfo.firstRoomPipeline = &firstRoomPipeline_;
  loopInfo.creativeViewModelPipeline = &creativeViewModelPipeline_;
  loopInfo.materialTexturePipeline = &materialTexturePipeline_;
  loopInfo.staticMeshInstancePipeline = &staticMeshInstancePipeline_;
  loopInfo.staticMeshInstanceMaterialPipeline =
      &materialTextureInstancePipeline_;
  loopInfo.firstRoomLayout = &firstRoomLayout_;
  loopInfo.materialTextureLayout = &materialTextureLayout_;
  loopInfo.firstRoomResources = &firstRoomResources_;
  loopInfo.frameCapture = &frameCapture_;
  loopInfo.externalUiHook = externalUiBridge_.recordHook();
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
    loopInfo.staticMeshInstancePipeline = &staticMeshInstancePipeline_;
    loopInfo.staticMeshInstanceMaterialPipeline =
        &materialTextureInstancePipeline_;
    loopInfo.firstRoomLayout = &firstRoomLayout_;
    loopInfo.materialTextureLayout = &materialTextureLayout_;
    loopInfo.firstRoomResources = &firstRoomResources_;
    loopInfo.frameCapture = &frameCapture_;
    // Both RenderLoopCreateInfo build sites must re-supply the hook or the
    // desktop UI disappears after the first window resize.
    loopInfo.externalUiHook = externalUiBridge_.recordHook();
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

void VulkanBackend::forwardExternalUiEvent(const SDL_Event& event) {
  externalUiBridge_.processEvent(event);
}

bool VulkanBackend::beginExternalUiFrame() {
  return externalUiBridge_.beginFrame();
}

bool VulkanBackend::externalUiFrameActive() const {
  return externalUiBridge_.frameActive();
}

bool VulkanBackend::externalUiRecordedLastFrame() const {
  return externalUiBridge_.recordedLastFrame();
}

bool VulkanBackend::externalUiWantsMouse() const {
  return externalUiBridge_.wantsMouse();
}

bool VulkanBackend::externalUiWantsKeyboard() const {
  return externalUiBridge_.wantsKeyboard();
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

VulkanStaticMeshAssetReloadResult VulkanBackend::reloadStaticMeshAssets() {
  VulkanStaticMeshAssetReloadResult result;
  if (lifecycleState_ != RendererLifecycleState::Ready ||
      !firstRoomReady_ || !firstRoomResources_.ready()) {
    result.reason = {"static_mesh_asset_reload_renderer_not_ready",
                     "static mesh asset reload renderer not ready"};
    result.receipt = makeReceipt("fail", result.reason.code);
    return result;
  }
  const RenderOutcome idle = waitIdle();
  if (idle != RenderOutcome::Ok) {
    result.outcome = idle;
    result.reason = {"static_mesh_asset_reload_wait_idle_failed",
                     "static mesh asset reload wait idle failed"};
    result.receipt = makeReceipt("fail", result.reason.code);
    return result;
  }

  const vulkan::BufferImageResourcesResult prepared =
      firstRoomResources_.prepareStaticMeshAssetReload();
  if (prepared.outcome != RenderOutcome::Ok) {
    result.outcome = prepared.outcome;
    result.reason = prepared.reason;
    result.receipt = prepared.receipt;
    return result;
  }

  StaticMeshMaterialPipelineBundle replacement;
  const VkDescriptorSetLayout textureLayout =
      firstRoomResources_
          .pendingStaticMeshMaterialTextures()
          .descriptorSetLayout;
  if (!createStaticMeshMaterialPipeline(textureLayout, replacement)) {
    firstRoomResources_.cancelStaticMeshAssetReload();
    result.outcome = RenderOutcome::PipelineOrShaderFailure;
    result.reason = {"static_mesh_asset_reload_pipeline_failed",
                     "static mesh asset reload pipeline failed"};
    result.receipt = makeReceipt("fail", result.reason.code);
    return result;
  }

  destroyStaticMeshMaterialPipeline();
  if (!firstRoomResources_.commitStaticMeshAssetReload()) {
    destroyStaticMeshMaterialPipelineBundle(replacement);
    firstRoomResources_.cancelStaticMeshAssetReload();
    static_cast<void>(initializeStaticMeshMaterialPipeline());
    result.outcome = RenderOutcome::FatalRendererError;
    result.reason = {"static_mesh_asset_reload_commit_failed",
                     "static mesh asset reload commit failed"};
    result.receipt = makeReceipt("fail", result.reason.code);
    return result;
  }
  materialTextureVertexShader_ = std::move(replacement.vertexShader);
  materialTextureInstanceVertexShader_ =
      std::move(replacement.instanceVertexShader);
  materialTextureFragmentShader_ = std::move(replacement.fragmentShader);
  materialTextureLayout_ = std::move(replacement.layout);
  materialTexturePipeline_ = std::move(replacement.pipeline);
  materialTextureInstancePipeline_ =
      std::move(replacement.instancePipeline);

  result.outcome = RenderOutcome::Ok;
  result.reason = {"static_mesh_asset_reload_applied",
                   "static mesh asset reload applied"};
  result.receipt = makeReceipt("pass", result.reason.code);
  appendReceiptField(
      result.receipt, "creative_preview_draw_count",
      static_cast<std::uint64_t>(
          firstRoomResources_.creativePreviewGeometry().indexedDraws.size()));
  appendReceiptField(
      result.receipt, "static_mesh_texture_count",
      static_cast<std::uint64_t>(
          firstRoomResources_.staticMeshMaterialTextures().textures.size()));
  diagnostics_ = result.receipt;
  return result;
}

void VulkanBackend::shutdown() {
  if (lifecycleState_ == RendererLifecycleState::Shutdown) {
    return;
  }
  renderLoop_.shutdown();
  waitIdle();
  externalUiBridge_.shutdown();
  destroyPacket7FirstRoomModules();
  commandRecording_.destroy();
  frameSync_.destroy();
  swapchain_.destroy();
  bootstrap_.shutdown();
  lifecycleState_ = RendererLifecycleState::Shutdown;
  diagnostics_ = makeReceipt("pass", "vulkan_backend_shutdown");
}

}  // namespace iggy3d
