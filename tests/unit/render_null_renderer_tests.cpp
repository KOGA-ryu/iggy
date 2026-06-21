#include "render/RendererApi.hpp"
#include "render/null/NullRenderer.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

iggy3d::FrameInput validFrame(iggy3d::SceneProjectionResult& scene,
                              iggy3d::DebugProjectionResult& debug) {
  scene.items.resize(2U);
  debug.items.resize(1U);
  iggy3d::FrameInput frame;
  frame.viewport = {640U, 360U, 640.0F / 360.0F};
  frame.clock = {1U, 1U, 0.0F, 1.0F / 60.0F};
  frame.camera.mode = iggy3d::RenderCameraMode::ThirdPerson;
  frame.camera.worldEye = {0.0F, 2.0F, 5.0F};
  frame.camera.worldForward = {0.0F, 0.0F, -1.0F};
  frame.camera.worldUp = {0.0F, 1.0F, 0.0F};
  frame.camera.nearPlane = 0.1F;
  frame.camera.farPlane = 200.0F;
  frame.projections.scene = &scene;
  frame.projections.debug = &debug;
  return frame;
}

bool constructsReadyAndReportsBackend() {
  iggy3d::NullRenderer renderer;
  const iggy3d::RenderReceipt receipt = renderer.diagnostics();
  return expect(renderer.lifecycleState() == iggy3d::RendererLifecycleState::Ready,
                "constructs ready") &&
         expect(renderer.backendKind() == iggy3d::RendererBackendKind::Null, "backend null") &&
         expect(iggy3d::hasReceiptField(receipt, "backend", "null"), "receipt backend") &&
         expect(iggy3d::hasReceiptField(receipt, "reason_code", "null_renderer_ok"),
                "initial reason");
}

bool acceptsValidDrawableFrameAndRecordsCounts() {
  iggy3d::NullRenderer renderer;
  iggy3d::SceneProjectionResult scene;
  iggy3d::DebugProjectionResult debug;
  const iggy3d::RenderSubmitResult result = renderer.submitFrame(validFrame(scene, debug));
  const iggy3d::RenderReceipt receipt = renderer.diagnostics();
  return expect(result.outcome == iggy3d::RenderOutcome::Ok, "valid frame ok") &&
         expect(result.reason.code == "null_renderer_ok", "valid reason") &&
         expect(iggy3d::hasReceiptField(result.receipt, "submitted_frame_count", "1"),
                "submitted count") &&
         expect(iggy3d::hasReceiptField(result.receipt, "accepted_frame_count", "1"),
                "accepted count") &&
         expect(iggy3d::hasReceiptField(result.receipt, "scene_item_count", "2"),
                "scene count") &&
         expect(iggy3d::hasReceiptField(result.receipt, "debug_item_count", "1"),
                "debug count") &&
         expect(iggy3d::hasReceiptField(result.receipt, "draw_count", "0"),
                "draw count zero") &&
         expect(iggy3d::hasReceiptField(receipt, "last_source_tick", "1"),
                "last source tick") &&
         expect(iggy3d::hasReceiptField(receipt, "last_frame_index", "1"), "last frame");
}

bool invalidAndNotDrawableFramesAreDiagnosed() {
  iggy3d::NullRenderer renderer;
  iggy3d::SceneProjectionResult scene;
  iggy3d::DebugProjectionResult debug;
  iggy3d::FrameInput notDrawable = validFrame(scene, debug);
  notDrawable.viewport.width = 0U;
  iggy3d::FrameInput invalid = validFrame(scene, debug);
  invalid.projections.scene = nullptr;

  const iggy3d::RenderSubmitResult skipped = renderer.submitFrame(notDrawable);
  const iggy3d::RenderSubmitResult rejected = renderer.submitFrame(invalid);

  return expect(skipped.outcome == iggy3d::RenderOutcome::SkipFrame, "not drawable skipped") &&
         expect(skipped.reason.code == "null_renderer_not_drawable", "not drawable reason") &&
         expect(iggy3d::hasReceiptField(skipped.receipt, "frame_reason_code",
                                        "frame_not_drawable"),
                "not drawable frame reason") &&
         expect(rejected.outcome == iggy3d::RenderOutcome::InvalidFrameInput,
                "invalid rejected") &&
         expect(rejected.reason.code == "null_renderer_frame_invalid", "invalid reason") &&
         expect(iggy3d::hasReceiptField(rejected.receipt, "rejected_frame_count", "2"),
                "rejected count");
}

bool resizeWaitAndShutdownBehave() {
  iggy3d::NullRenderer renderer;
  const iggy3d::RenderSubmitResult resized =
      renderer.resize({320U, 200U, 320.0F / 200.0F});
  const iggy3d::RenderSubmitResult minimized = renderer.resize({0U, 200U, 1.0F});
  const iggy3d::RenderOutcome idle = renderer.waitIdle();
  renderer.shutdown();
  renderer.shutdown();

  iggy3d::SceneProjectionResult scene;
  iggy3d::DebugProjectionResult debug;
  const iggy3d::RenderSubmitResult afterShutdown = renderer.submitFrame(validFrame(scene, debug));

  return expect(resized.outcome == iggy3d::RenderOutcome::Ok, "resize ok") &&
         expect(iggy3d::hasReceiptField(resized.receipt, "last_viewport_width", "320"),
                "resize width") &&
         expect(minimized.outcome == iggy3d::RenderOutcome::SkipFrame, "minimized skip") &&
         expect(minimized.reason.code == "null_renderer_resize_not_drawable",
                "minimized reason") &&
         expect(idle == iggy3d::RenderOutcome::Ok, "wait idle") &&
         expect(renderer.lifecycleState() == iggy3d::RendererLifecycleState::Shutdown,
                "shutdown lifecycle") &&
         expect(afterShutdown.outcome == iggy3d::RenderOutcome::RendererNotReady,
                "submit after shutdown") &&
         expect(afterShutdown.reason.code == "null_renderer_shutdown", "shutdown reason");
}

bool factoryCreatesNullRendererNow() {
  iggy3d::RendererCreateInfo create;
  create.backend = iggy3d::RendererBackendKind::Null;
  iggy3d::RendererApi api = iggy3d::createRenderer(create);
  iggy3d::SceneProjectionResult scene;
  iggy3d::DebugProjectionResult debug;
  const iggy3d::RenderSubmitResult result = api.submitFrame(validFrame(scene, debug));

  return expect(api.hasBackend(), "factory has backend") &&
         expect(api.lifecycleState() == iggy3d::RendererLifecycleState::Ready, "factory ready") &&
         expect(result.outcome == iggy3d::RenderOutcome::Ok, "factory submit ok") &&
         expect(result.reason.code == "null_renderer_ok", "factory null reason") &&
         expect(!iggy3d::hasReceiptField(api.diagnostics(), "reason_code",
                                         "renderer_missing_backend"),
                "missing backend gone");
}

}  // namespace

int main() {
  bool ok = true;
  ok = constructsReadyAndReportsBackend() && ok;
  ok = acceptsValidDrawableFrameAndRecordsCounts() && ok;
  ok = invalidAndNotDrawableFramesAreDiagnosed() && ok;
  ok = resizeWaitAndShutdownBehave() && ok;
  ok = factoryCreatesNullRendererNow() && ok;
  return ok ? 0 : 1;
}
