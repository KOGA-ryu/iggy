#include "render/null/NullRenderer.hpp"

namespace iggy3d {

NullRenderer::NullRenderer() = default;
NullRenderer::~NullRenderer() = default;

RendererBackendKind NullRenderer::backendKind() const {
  return RendererBackendKind::Null;
}

RendererLifecycleState NullRenderer::lifecycleState() const {
  return lifecycleState_;
}

RenderReceipt NullRenderer::makeReceipt(std::string_view result,
                                        std::string_view reasonCode,
                                        std::string_view frameInputValid) const {
  RenderReceipt receipt;
  appendReceiptField(receipt, "receipt_version", "1");
  appendReceiptField(receipt, "repo", "iggy3d");
  appendReceiptField(receipt, "file_plan", "src/render/null/NullRenderer.cpp");
  appendReceiptField(receipt, "packet_order", "2");
  appendReceiptField(receipt, "allowed_to_implement_code_now", "false");
  appendReceiptField(receipt, "backend", "null");
  appendReceiptField(receipt, "renderer_lifecycle", rendererLifecycleStateName(lifecycleState_));
  appendReceiptField(receipt, "submitted_frame_count", submittedFrameCount_);
  appendReceiptField(receipt, "accepted_frame_count", acceptedFrameCount_);
  appendReceiptField(receipt, "rejected_frame_count", rejectedFrameCount_);
  appendReceiptField(receipt, "last_source_tick", lastSourceTick_);
  appendReceiptField(receipt, "last_frame_index", lastFrameIndex_);
  appendReceiptField(receipt, "last_viewport_width", static_cast<std::uint64_t>(lastViewportWidth_));
  appendReceiptField(receipt, "last_viewport_height",
                     static_cast<std::uint64_t>(lastViewportHeight_));
  appendReceiptField(receipt, "scene_item_count", lastSceneItemCount_);
  appendReceiptField(receipt, "debug_item_count", lastDebugItemCount_);
  appendReceiptField(receipt, "draw_count", static_cast<std::uint64_t>(0));
  appendReceiptField(receipt, "frame_input_valid", frameInputValid);
  appendReceiptField(receipt, "runtime_hash_before", "unavailable");
  appendReceiptField(receipt, "runtime_hash_after", "unavailable");
  appendReceiptField(receipt, "replay_invariant", "unavailable");
  appendReceiptField(receipt, "result", result);
  appendReceiptField(receipt, "reason_code", reasonCode);
  return receipt;
}

RenderSubmitResult NullRenderer::submitFrame(const FrameInput& frame) {
  if (lifecycleState_ == RendererLifecycleState::Shutdown) {
    lastReasonCode_ = "null_renderer_shutdown";
    RenderSubmitResult result;
    result.outcome = RenderOutcome::RendererNotReady;
    result.reason = {lastReasonCode_, "null renderer shutdown"};
    result.receipt = makeReceipt("fail", lastReasonCode_, "unavailable");
    return result;
  }

  ++submittedFrameCount_;
  const FrameInputStatus frameStatus = validateFrameInput(frame);
  if (frameStatus == FrameInputStatus::Valid) {
    ++acceptedFrameCount_;
    lastSourceTick_ = frame.clock.sourceTick;
    lastFrameIndex_ = frame.clock.frameIndex;
    lastViewportWidth_ = frame.viewport.width;
    lastViewportHeight_ = frame.viewport.height;
    lastSceneItemCount_ = static_cast<std::uint64_t>(frame.projections.scene->items.size());
    lastDebugItemCount_ = frame.projections.debug == nullptr
                              ? 0U
                              : static_cast<std::uint64_t>(frame.projections.debug->items.size());
    lastReasonCode_ = "null_renderer_ok";

    RenderSubmitResult result;
    result.outcome = RenderOutcome::Ok;
    result.reason = {lastReasonCode_, "null renderer ok"};
    result.receipt = makeReceipt("pass", lastReasonCode_, "true");
    return result;
  }

  ++rejectedFrameCount_;
  if (frameStatus == FrameInputStatus::NotDrawable) {
    lastReasonCode_ = "null_renderer_not_drawable";
    RenderSubmitResult result;
    result.outcome = RenderOutcome::SkipFrame;
    result.reason = {lastReasonCode_, "null renderer not drawable"};
    result.receipt = makeReceipt("skip", lastReasonCode_, "false");
    appendReceiptField(result.receipt, "frame_reason_code", frameInputReasonCode(frameStatus));
    return result;
  }

  lastReasonCode_ = "null_renderer_frame_invalid";
  RenderSubmitResult result;
  result.outcome = RenderOutcome::InvalidFrameInput;
  result.reason = {lastReasonCode_, "null renderer frame invalid"};
  result.receipt = makeReceipt("fail", lastReasonCode_, "false");
  appendReceiptField(result.receipt, "frame_reason_code", frameInputReasonCode(frameStatus));
  return result;
}

RenderSubmitResult NullRenderer::resize(RenderViewport viewport) {
  if (viewport.width == 0U || viewport.height == 0U) {
    lastReasonCode_ = "null_renderer_resize_not_drawable";
    RenderSubmitResult result;
    result.outcome = RenderOutcome::SkipFrame;
    result.reason = {lastReasonCode_, "null renderer resize not drawable"};
    result.receipt = makeReceipt("skip", lastReasonCode_, "false");
    return result;
  }

  lastViewportWidth_ = viewport.width;
  lastViewportHeight_ = viewport.height;
  lastReasonCode_ = "null_renderer_ok";
  RenderSubmitResult result;
  result.outcome = RenderOutcome::Ok;
  result.reason = {lastReasonCode_, "null renderer ok"};
  result.receipt = makeReceipt("pass", lastReasonCode_, "unavailable");
  return result;
}

RenderReceipt NullRenderer::diagnostics() const {
  return makeReceipt("pass", lastReasonCode_, "unavailable");
}

RenderOutcome NullRenderer::waitIdle() {
  return RenderOutcome::Ok;
}

void NullRenderer::shutdown() {
  lifecycleState_ = RendererLifecycleState::Shutdown;
  lastReasonCode_ = "null_renderer_shutdown";
}

}  // namespace iggy3d
