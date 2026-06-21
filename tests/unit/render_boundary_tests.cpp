#include "render/FrameInput.hpp"
#include "render/RenderBackend.hpp"
#include "render/RenderDiagnostics.hpp"
#include "render/RendererApi.hpp"
#include "render/RendererConfig.hpp"

#include <iostream>
#include <memory>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

iggy3d::FrameInput validFrame(const iggy3d::SceneProjectionResult& scene) {
  iggy3d::FrameInput frame;
  frame.viewport = {800U, 600U, 800.0F / 600.0F};
  frame.clock = {1U, 1U, 0.0F, 0.0F};
  frame.camera.worldEye = {0.0F, 2.0F, 5.0F};
  frame.camera.worldForward = {0.0F, 0.0F, -1.0F};
  frame.camera.worldUp = {0.0F, 1.0F, 0.0F};
  frame.projections.scene = &scene;
  return frame;
}

iggy3d::RenderSubmitResult okSubmitResult(std::string_view reasonCode) {
  iggy3d::RenderSubmitResult result;
  result.outcome = iggy3d::RenderOutcome::Ok;
  result.reason = {reasonCode, "test backend ok"};
  iggy3d::appendReceiptField(result.receipt, "receipt_version", "1");
  iggy3d::appendReceiptField(result.receipt, "repo", "iggy3d");
  iggy3d::appendReceiptField(result.receipt, "backend", "null");
  iggy3d::appendReceiptField(result.receipt, "result", "pass");
  iggy3d::appendReceiptField(result.receipt, "reason_code", reasonCode);
  return result;
}

class TestBackend final : public iggy3d::RenderBackend {
public:
  iggy3d::RendererBackendKind backendKind() const override {
    return iggy3d::RendererBackendKind::Null;
  }

  iggy3d::RendererLifecycleState lifecycleState() const override {
    return state;
  }

  iggy3d::RenderSubmitResult submitFrame(const iggy3d::FrameInput& frame) override {
    ++submitCount;
    lastFrameTick = frame.clock.sourceTick;
    return okSubmitResult("renderer_ok");
  }

  iggy3d::RenderSubmitResult resize(iggy3d::RenderViewport viewport) override {
    ++resizeCount;
    lastWidth = viewport.width;
    return okSubmitResult("renderer_ok");
  }

  iggy3d::RenderReceipt diagnostics() const override {
    iggy3d::RenderReceipt receipt;
    iggy3d::appendReceiptField(receipt, "reason_code", "renderer_ok");
    return receipt;
  }

  iggy3d::RenderOutcome waitIdle() override {
    ++waitCount;
    return iggy3d::RenderOutcome::Ok;
  }

  void shutdown() override {
    ++shutdownCount;
    state = iggy3d::RendererLifecycleState::Shutdown;
  }

  iggy3d::RendererLifecycleState state = iggy3d::RendererLifecycleState::Ready;
  int submitCount = 0;
  int resizeCount = 0;
  int waitCount = 0;
  int shutdownCount = 0;
  std::uint64_t lastFrameTick = 0;
  std::uint32_t lastWidth = 0;
};

bool injectedBackendIsUsed() {
  auto backend = std::make_unique<TestBackend>();
  TestBackend* rawBackend = backend.get();
  iggy3d::RendererApi api{std::move(backend)};
  iggy3d::SceneProjectionResult scene;
  iggy3d::FrameInput frame = validFrame(scene);
  const iggy3d::RenderSubmitResult submit = api.submitFrame(frame);
  const iggy3d::RenderSubmitResult resize = api.resize({1024U, 768U, 1024.0F / 768.0F});
  const iggy3d::RenderOutcome wait = api.waitIdle();

  return expect(api.hasBackend(), "api has backend") &&
         expect(api.lifecycleState() == iggy3d::RendererLifecycleState::Ready, "api ready") &&
         expect(submit.outcome == iggy3d::RenderOutcome::Ok, "submit ok") &&
         expect(resize.outcome == iggy3d::RenderOutcome::Ok, "resize ok") &&
         expect(wait == iggy3d::RenderOutcome::Ok, "wait ok") &&
         expect(rawBackend->submitCount == 1, "submit forwarded") &&
         expect(rawBackend->resizeCount == 1 && rawBackend->lastWidth == 1024U,
                "resize forwarded") &&
         expect(rawBackend->waitCount == 1, "wait forwarded");
}

bool factoryNullBackendIsAvailableAndVulkanIsStillDiagnosed() {
  iggy3d::RendererCreateInfo create;
  create.backend = iggy3d::RendererBackendKind::Null;
  const iggy3d::RendererApi nullApi = iggy3d::createRenderer(create);

  iggy3d::RendererCreateInfo vulkanCreate;
  vulkanCreate.backend = iggy3d::RendererBackendKind::Vulkan;
  const iggy3d::RendererApi vulkanApi = iggy3d::createRenderer(vulkanCreate);

  return expect(nullApi.hasBackend(), "null backend present") &&
         expect(iggy3d::hasReceiptField(nullApi.diagnostics(), "reason_code", "renderer_ok"),
                "null backend ready") &&
         expect(!vulkanApi.hasBackend(), "vulkan backend absent") &&
         expect(iggy3d::hasReceiptField(vulkanApi.diagnostics(), "reason_code",
                                        "vulkan_not_built"),
                "vulkan not built");
}

bool invalidFrameAndShutdownAreDiagnosed() {
  auto backend = std::make_unique<TestBackend>();
  TestBackend* rawBackend = backend.get();
  iggy3d::RendererApi api{std::move(backend)};
  iggy3d::FrameInput invalidFrame;
  const iggy3d::RenderSubmitResult invalid = api.submitFrame(invalidFrame);
  api.shutdown();
  api.shutdown();
  const iggy3d::RenderSubmitResult afterShutdown = api.submitFrame(invalidFrame);

  return expect(invalid.outcome == iggy3d::RenderOutcome::InvalidFrameInput,
                "invalid frame outcome") &&
         expect(invalid.reason.code == "frame_input_invalid", "invalid frame reason") &&
         expect(iggy3d::hasReceiptField(invalid.receipt, "frame_reason_code",
                                        "frame_not_drawable"),
                "invalid frame detail") &&
         expect(rawBackend->submitCount == 0, "invalid frame not forwarded") &&
         expect(rawBackend->shutdownCount == 1, "shutdown idempotent") &&
         expect(afterShutdown.reason.code == "renderer_shutdown", "submit after shutdown");
}

bool publicNamesAreStable() {
  return expect(iggy3d::rendererBackendKindName(iggy3d::RendererBackendKind::Null) == "null",
                "null backend name") &&
         expect(iggy3d::rendererLifecycleStateName(iggy3d::RendererLifecycleState::Ready) ==
                    "ready",
                "ready lifecycle name") &&
         expect(iggy3d::rendererLifecycleStateName(iggy3d::RendererLifecycleState::Shutdown) ==
                    "shutdown",
                "shutdown lifecycle name");
}

}  // namespace

int main() {
  bool ok = true;
  ok = injectedBackendIsUsed() && ok;
  ok = factoryNullBackendIsAvailableAndVulkanIsStillDiagnosed() && ok;
  ok = invalidFrameAndShutdownAreDiagnosed() && ok;
  ok = publicNamesAreStable() && ok;
  return ok ? 0 : 1;
}
