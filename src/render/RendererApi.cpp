#include "render/RendererApi.hpp"

#include "render/RenderBackend.hpp"
#include "render/null/NullRenderer.hpp"

#include <utility>

namespace iggy3d {
namespace {

RenderReason rendererReason(std::string_view code) {
  if (code == "renderer_ok") {
    return {code, "renderer ok"};
  }
  if (code == "renderer_missing_backend") {
    return {code, "renderer backend missing"};
  }
  if (code == "renderer_shutdown") {
    return {code, "renderer shutdown"};
  }
  if (code == "frame_input_invalid") {
    return {code, "frame input invalid"};
  }
  if (code == "vulkan_not_built") {
    return {code, "vulkan backend not built"};
  }
  if (code == "vulkan_surface_provider_missing") {
    return {code, "vulkan surface provider missing"};
  }
  if (code == "backend_submit_failed") {
    return {code, "backend submit failed"};
  }
  return {"renderer_not_ready", "renderer not ready"};
}

RenderReceipt makeReceipt(RendererBackendKind backend,
                          std::string_view reasonCode,
                          std::string_view frameInputValid,
                          std::string_view result) {
  RenderReceipt receipt;
  appendReceiptField(receipt, "receipt_version", "1");
  appendReceiptField(receipt, "repo", "iggy3d");
  appendReceiptField(receipt, "file_plan", "src/render/RendererApi.cpp");
  appendReceiptField(receipt, "packet_order", "1");
  appendReceiptField(receipt, "allowed_to_implement_code_now", "false");
  appendReceiptField(receipt, "backend", rendererBackendKindName(backend));
  appendReceiptField(receipt, "frame_input_valid", frameInputValid);
  appendReceiptField(receipt, "runtime_hash_before", "unavailable");
  appendReceiptField(receipt, "runtime_hash_after", "unavailable");
  appendReceiptField(receipt, "replay_invariant", "unavailable");
  appendReceiptField(receipt, "strict_vulkan", "false");
  appendReceiptField(receipt, "result", result);
  appendReceiptField(receipt, "reason_code", reasonCode);
  return receipt;
}

RenderSubmitResult makeSubmitResult(RenderOutcome outcome,
                                    std::string_view reasonCode,
                                    RendererBackendKind backend,
                                    std::string_view frameInputValid,
                                    std::string_view result) {
  RenderSubmitResult submit;
  submit.outcome = outcome;
  submit.reason = rendererReason(reasonCode);
  submit.receipt = makeReceipt(backend, submit.reason.code, frameInputValid, result);
  return submit;
}

RendererBackendKind backendKindFor(const RenderBackend* backend) {
  return backend == nullptr ? RendererBackendKind::Null : backend->backendKind();
}

}  // namespace

std::string_view rendererBackendKindName(RendererBackendKind kind) {
  switch (kind) {
    case RendererBackendKind::Null:
      return "null";
    case RendererBackendKind::Vulkan:
      return "vulkan";
  }
  return "unavailable";
}

std::string_view rendererLifecycleStateName(RendererLifecycleState state) {
  switch (state) {
    case RendererLifecycleState::NotInitialized:
      return "not_initialized";
    case RendererLifecycleState::Ready:
      return "ready";
    case RendererLifecycleState::DeviceLost:
      return "device_lost";
    case RendererLifecycleState::Shutdown:
      return "shutdown";
  }
  return "not_initialized";
}

RendererApi::RendererApi()
    : diagnostics_(makeReceipt(RendererBackendKind::Null, "renderer_missing_backend",
                               "unavailable", "fail")) {}

RendererApi::RendererApi(RenderReceipt diagnostics) : diagnostics_(std::move(diagnostics)) {}

RendererApi::RendererApi(std::unique_ptr<RenderBackend> backend)
    : backend_(std::move(backend)),
      lifecycleState_(backend_ == nullptr ? RendererLifecycleState::NotInitialized
                                          : backend_->lifecycleState()),
      diagnostics_(makeReceipt(backendKindFor(backend_.get()),
                               backend_ == nullptr ? "renderer_missing_backend" : "renderer_ok",
                               "unavailable", backend_ == nullptr ? "fail" : "pass")) {}

RendererApi::~RendererApi() = default;
RendererApi::RendererApi(RendererApi&&) noexcept = default;
RendererApi& RendererApi::operator=(RendererApi&&) noexcept = default;

RenderSubmitResult RendererApi::submitFrame(const FrameInput& frame) {
  if (lifecycleState_ == RendererLifecycleState::Shutdown) {
    RenderSubmitResult result = makeSubmitResult(RenderOutcome::RendererNotReady,
                                                 "renderer_shutdown", backendKindFor(backend_.get()),
                                                 "unavailable", "fail");
    diagnostics_ = result.receipt;
    return result;
  }
  if (backend_ == nullptr) {
    RenderSubmitResult result =
        makeSubmitResult(RenderOutcome::RendererNotReady, "renderer_missing_backend",
                         RendererBackendKind::Null, "unavailable", "fail");
    diagnostics_ = result.receipt;
    return result;
  }
  if (lifecycleState_ != RendererLifecycleState::Ready ||
      backend_->lifecycleState() != RendererLifecycleState::Ready) {
    RenderSubmitResult result =
        makeSubmitResult(RenderOutcome::RendererNotReady, "renderer_not_ready",
                         backend_->backendKind(), "unavailable", "fail");
    diagnostics_ = result.receipt;
    return result;
  }

  const FrameInputStatus frameStatus = validateFrameInput(frame);
  if (frameStatus != FrameInputStatus::Valid) {
    RenderSubmitResult result =
        makeSubmitResult(RenderOutcome::InvalidFrameInput, "frame_input_invalid",
                         backend_->backendKind(), "false", "fail");
    appendReceiptField(result.receipt, "frame_reason_code", frameInputReasonCode(frameStatus));
    diagnostics_ = result.receipt;
    return result;
  }

  RenderSubmitResult result = backend_->submitFrame(frame);
  if (result.outcome != RenderOutcome::Ok) {
    result.reason = rendererReason("backend_submit_failed");
    appendReceiptField(result.receipt, "api_reason_code", result.reason.code);
  }
  diagnostics_ = result.receipt;
  return result;
}

RenderSubmitResult RendererApi::resize(RenderViewport viewport) {
  if (lifecycleState_ == RendererLifecycleState::Shutdown) {
    RenderSubmitResult result = makeSubmitResult(RenderOutcome::RendererNotReady,
                                                 "renderer_shutdown", backendKindFor(backend_.get()),
                                                 "unavailable", "fail");
    diagnostics_ = result.receipt;
    return result;
  }
  if (backend_ == nullptr) {
    RenderSubmitResult result =
        makeSubmitResult(RenderOutcome::RendererNotReady, "renderer_missing_backend",
                         RendererBackendKind::Null, "unavailable", "fail");
    diagnostics_ = result.receipt;
    return result;
  }
  RenderSubmitResult result = backend_->resize(viewport);
  diagnostics_ = result.receipt;
  return result;
}

RenderReceipt RendererApi::diagnostics() const {
  return diagnostics_;
}

RenderOutcome RendererApi::waitIdle() {
  if (lifecycleState_ == RendererLifecycleState::Shutdown) {
    return RenderOutcome::Ok;
  }
  if (backend_ == nullptr) {
    diagnostics_ =
        makeReceipt(RendererBackendKind::Null, "renderer_missing_backend", "unavailable", "fail");
    return RenderOutcome::RendererNotReady;
  }
  return backend_->waitIdle();
}

void RendererApi::shutdown() {
  if (lifecycleState_ == RendererLifecycleState::Shutdown) {
    return;
  }
  if (backend_ != nullptr) {
    backend_->shutdown();
  }
  lifecycleState_ = RendererLifecycleState::Shutdown;
  diagnostics_ = makeReceipt(backendKindFor(backend_.get()), "renderer_shutdown", "unavailable",
                             "pass");
}

RendererLifecycleState RendererApi::lifecycleState() const {
  return lifecycleState_;
}

bool RendererApi::hasBackend() const {
  return backend_ != nullptr;
}

RendererApi createRenderer(const RendererCreateInfo& createInfo) {
  const RendererConfigResult config = resolveRendererConfig(createInfo.config);
  if (config.outcome != RenderOutcome::Ok) {
    return RendererApi{config.receipt};
  }

  if (createInfo.backend == RendererBackendKind::Vulkan) {
    return RendererApi{makeReceipt(RendererBackendKind::Vulkan, "vulkan_surface_provider_missing",
                                   "unavailable", "fail")};
  }

  return RendererApi{std::make_unique<NullRenderer>()};
}

}  // namespace iggy3d
