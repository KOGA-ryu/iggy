#include "projection/scene/SceneProjection.hpp"
#include "render/FrameInput.hpp"
#include "render/RenderDiagnostics.hpp"

#include <iostream>
#include <string_view>
#include <utility>

#if defined(IGGY3D_HAS_SDL3) && defined(IGGY3D_HAS_VULKAN)
#include "app/platform/SdlVulkanSurface.hpp"
#include "app/platform/SdlWindow.hpp"
#include "render/vulkan/VulkanBackend.hpp"
#endif

namespace {

bool strictLane() {
#if defined(IGGY3D_REQUIRE_VULKAN_SMOKE_ENABLED)
  return true;
#else
  return false;
#endif
}

iggy3d::RenderReceipt baseReceipt(std::string_view result, std::string_view reason) {
  iggy3d::RenderReceipt receipt;
  iggy3d::appendReceiptField(receipt, "receipt_version", "1");
  iggy3d::appendReceiptField(receipt, "repo", "iggy3d");
  iggy3d::appendReceiptField(receipt, "smoke", "vulkan_resize_minimize");
  iggy3d::appendReceiptField(receipt, "packet_order", "5");
  iggy3d::appendReceiptField(receipt, "backend", "vulkan");
  iggy3d::appendReceiptField(receipt, "zero_extent_skip", false);
  iggy3d::appendReceiptField(receipt, "restored_extent_recreated", false);
  iggy3d::appendReceiptField(receipt, "result", result);
  iggy3d::appendReceiptField(receipt, "reason_code", reason);
  return receipt;
}

int printReceipt(const iggy3d::RenderReceipt& receipt, int exitCode) {
  std::cout << iggy3d::formatRenderReceipt(receipt);
  return exitCode;
}

#if defined(IGGY3D_HAS_SDL3) && defined(IGGY3D_HAS_VULKAN)
iggy3d::FrameInput validFrame(const iggy3d::SceneProjectionResult& scene,
                              std::uint32_t width,
                              std::uint32_t height) {
  iggy3d::FrameInput frame;
  frame.viewport = {width, height, static_cast<float>(width) / static_cast<float>(height)};
  frame.clock = {0U, 1U, 0.0F, 1.0F / 60.0F};
  frame.camera.mode = iggy3d::RenderCameraMode::ThirdPerson;
  frame.camera.worldEye = {0.0F, 2.0F, 5.0F};
  frame.camera.worldForward = {0.0F, 0.0F, -1.0F};
  frame.camera.worldUp = {0.0F, 1.0F, 0.0F};
  frame.camera.nearPlane = 0.1F;
  frame.camera.farPlane = 200.0F;
  frame.projections.scene = &scene;
  return frame;
}
#endif

}  // namespace

int main() {
#if !defined(IGGY3D_HAS_VULKAN)
  return printReceipt(baseReceipt(strictLane() ? "fail" : "skip",
                                  strictLane() ? "vulkan_smoke_strict_dependency_missing"
                                               : "vulkan_smoke_skipped_loader_missing"),
                      strictLane() ? 1 : 77);
#elif !defined(IGGY3D_HAS_SDL3)
  return printReceipt(baseReceipt(strictLane() ? "fail" : "skip",
                                  strictLane() ? "vulkan_smoke_strict_dependency_missing"
                                               : "vulkan_smoke_skipped_sdl_missing"),
                      strictLane() ? 1 : 77);
#else
  iggy3d::SdlWindowCreateInfo create;
  create.title = "iggy3d resize smoke";
  create.width = 640U;
  create.height = 360U;
  create.vulkan = true;
  iggy3d::SdlWindow window(create);
  window.pollEvents();
  if (!window.isOpen() || !window.isDrawable()) {
    return printReceipt(baseReceipt(strictLane() ? "fail" : "skip",
                                    strictLane() ? "vulkan_smoke_strict_dependency_missing"
                                                 : "vulkan_smoke_skipped_no_display"),
                        strictLane() ? 1 : 77);
  }
  iggy3d::SdlVulkanSurfaceProvider sdlProvider;
  const iggy3d::SdlVulkanExtensionList extensions =
      sdlProvider.requiredInstanceExtensions(window);
  if (extensions.outcome != iggy3d::RenderOutcome::Ok) {
    return printReceipt(baseReceipt(strictLane() ? "fail" : "skip",
                                    strictLane() ? "vulkan_smoke_strict_dependency_missing"
                                                 : "vulkan_smoke_skipped_no_display"),
                        strictLane() ? 1 : 77);
  }
  const iggy3d::SdlDrawableExtent extent = window.drawableExtent();
  iggy3d::VulkanBackendCreateInfo backendInfo;
  backendInfo.config.allowSoftwareVulkan = true;
  backendInfo.drawableWidth = extent.width;
  backendInfo.drawableHeight = extent.height;
  backendInfo.surfaceProvider.requiredInstanceExtensions = extensions.names;
  backendInfo.surfaceProvider.createSurface =
      [&sdlProvider, &window](VkInstance instance, VkSurfaceKHR* surface) {
        const iggy3d::SdlVulkanSurfaceCreateResult created =
            sdlProvider.createSurface(window, instance);
        if (created.outcome == iggy3d::RenderOutcome::Ok && surface != nullptr) {
          *surface = created.surface;
        }
        iggy3d::RenderReceipt receipt;
        iggy3d::appendReceiptField(receipt, "result",
                                   created.outcome == iggy3d::RenderOutcome::Ok ? "pass" : "fail");
        iggy3d::appendReceiptField(receipt, "reason_code", created.reason.code);
        return receipt;
      };
  iggy3d::VulkanBackend backend(std::move(backendInfo));
  if (backend.lifecycleState() != iggy3d::RendererLifecycleState::Ready) {
    return printReceipt(baseReceipt(strictLane() ? "fail" : "skip",
                                    strictLane() ? "vulkan_smoke_strict_dependency_missing"
                                                 : "vulkan_smoke_skipped_no_suitable_device"),
                        strictLane() ? 1 : 77);
  }
  const iggy3d::RenderSubmitResult zero =
      backend.resize({0U, 0U, 1.0F});
  const iggy3d::RenderSubmitResult restored =
      backend.resize({extent.width, extent.height,
                      static_cast<float>(extent.width) / static_cast<float>(extent.height)});
  iggy3d::SceneProjectionResult scene;
  const iggy3d::RenderSubmitResult submitted =
      backend.submitFrame(validFrame(scene, extent.width, extent.height));
  iggy3d::RenderReceipt receipt = submitted.receipt;
  iggy3d::appendReceiptField(receipt, "smoke", "vulkan_resize_minimize");
  iggy3d::appendReceiptField(receipt, "zero_extent_skip",
                             zero.outcome == iggy3d::RenderOutcome::SkipFrame);
  iggy3d::appendReceiptField(receipt, "restored_swapchain_ready",
                             iggy3d::hasReceiptField(restored.receipt, "swapchain_state",
                                                     "ready"));
  iggy3d::appendReceiptField(receipt, "restored_extent_recreated",
                             restored.outcome == iggy3d::RenderOutcome::Ok &&
                                 iggy3d::hasReceiptField(restored.receipt, "swapchain_state",
                                                         "ready"));
  iggy3d::appendReceiptField(receipt, "post_restore_submit_ok",
                             submitted.outcome == iggy3d::RenderOutcome::Ok);
  backend.shutdown();
  const bool ok = zero.outcome == iggy3d::RenderOutcome::SkipFrame &&
                  restored.outcome == iggy3d::RenderOutcome::Ok &&
                  iggy3d::hasReceiptField(restored.receipt, "swapchain_state", "ready") &&
                  submitted.outcome == iggy3d::RenderOutcome::Ok;
  return printReceipt(ok ? receipt : baseReceipt("fail", "vulkan_smoke_receipt_invalid"),
                      ok ? 0 : 1);
#endif
}
