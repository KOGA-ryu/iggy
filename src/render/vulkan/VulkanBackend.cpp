#include "render/vulkan/VulkanBackend.hpp"

namespace iggy3d {
namespace {

RenderReason backendReason(std::string_view code) {
  if (code == "vulkan_backend_no_swapchain_yet") {
    return {code, "vulkan backend has no swapchain yet"};
  }
  if (code == "vulkan_backend_shutdown") {
    return {code, "vulkan backend shutdown"};
  }
  if (code == "vulkan_smoke_pass") {
    return {code, "vulkan smoke pass"};
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
  lifecycleState_ = bootstrap_.ready() ? RendererLifecycleState::Ready
                                       : RendererLifecycleState::NotInitialized;
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
  appendReceiptField(receipt, "packet_order", "4");
  appendReceiptField(receipt, "allowed_to_implement_code_now", "false");
  appendReceiptField(receipt, "backend", "vulkan");
  appendReceiptField(receipt, "backend_phase", "bootstrap");
  appendReceiptField(receipt, "initialized", bootstrap_.ready());
  appendReceiptField(receipt, "device_ready", bootstrap_.ready());
  appendReceiptField(receipt, "surface_ready", bootstrap_.handles().surface != VkSurfaceKHR{});
  appendReceiptField(receipt, "validation", "unavailable");
  appendReceiptField(receipt, "sync_validation", "unavailable");
  appendReceiptField(receipt, "function_loading_clean", bootstrap_.functions().clean);
  appendReceiptField(receipt, "last_resize_width", static_cast<std::uint64_t>(lastResize_.width));
  appendReceiptField(receipt, "last_resize_height", static_cast<std::uint64_t>(lastResize_.height));
  appendReceiptField(receipt, "result", result);
  appendReceiptField(receipt, "reason_code", reasonCode);
  return receipt;
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
  result.outcome = RenderOutcome::SkipFrame;
  result.reason = backendReason("vulkan_backend_no_swapchain_yet");
  result.receipt = makeReceipt("skip", result.reason.code);
  diagnostics_ = result.receipt;
  return result;
}

RenderSubmitResult VulkanBackend::resize(RenderViewport viewport) {
  lastResize_ = viewport;
  RenderSubmitResult result;
  result.outcome = RenderOutcome::SkipFrame;
  result.reason = backendReason("vulkan_backend_no_swapchain_yet");
  result.receipt = makeReceipt("skip", result.reason.code);
  diagnostics_ = result.receipt;
  return result;
}

RenderReceipt VulkanBackend::diagnostics() const {
  return diagnostics_;
}

RenderOutcome VulkanBackend::waitIdle() {
#if defined(IGGY3D_HAS_VULKAN)
  if (bootstrap_.handles().device != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(bootstrap_.handles().device);
  }
#endif
  return RenderOutcome::Ok;
}

void VulkanBackend::shutdown() {
  if (lifecycleState_ == RendererLifecycleState::Shutdown) {
    return;
  }
  bootstrap_.shutdown();
  lifecycleState_ = RendererLifecycleState::Shutdown;
  diagnostics_ = makeReceipt("pass", "vulkan_backend_shutdown");
}

}  // namespace iggy3d
