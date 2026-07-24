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

std::string_view platformName() {
#if defined(__APPLE__)
  return "macos";
#elif defined(__linux__)
  return "linux";
#elif defined(_WIN32)
  return "windows";
#else
  return "unknown";
#endif
}

int printReceipt(const iggy3d::RenderReceipt& receipt, int exitCode) {
  std::cout << iggy3d::formatRenderReceipt(receipt);
  return exitCode;
}

iggy3d::RenderReceipt baseReceipt(std::string_view result, std::string_view reason) {
  iggy3d::RenderReceipt receipt;
  iggy3d::appendReceiptField(receipt, "receipt_version", "1");
  iggy3d::appendReceiptField(receipt, "repo", "iggy3d");
  iggy3d::appendReceiptField(receipt, "smoke", "vulkan_device");
  iggy3d::appendReceiptField(receipt, "strict_lane", strictLane());
  iggy3d::appendReceiptField(receipt, "platform", platformName());
  iggy3d::appendReceiptField(receipt, "backend", "vulkan");
  iggy3d::appendReceiptField(receipt, "loader_strategy", "linked_loader");
  iggy3d::appendReceiptField(receipt, "physical_device_count", static_cast<std::uint64_t>(0));
  iggy3d::appendReceiptField(receipt, "selected_physical_device_index", "none");
  iggy3d::appendReceiptField(receipt, "device_name", "none");
  iggy3d::appendReceiptField(receipt, "device_type", "none");
  iggy3d::appendReceiptField(receipt, "api_version_selected", "none");
  iggy3d::appendReceiptField(receipt, "driver_version", "none");
  iggy3d::appendReceiptField(receipt, "graphics_queue_family", "none");
  iggy3d::appendReceiptField(receipt, "present_queue_family", "none");
  iggy3d::appendReceiptField(receipt, "enabled_instance_extensions", "");
  iggy3d::appendReceiptField(receipt, "enabled_device_extensions", "");
  iggy3d::appendReceiptField(receipt, "dynamic_rendering_source", "unavailable");
  iggy3d::appendReceiptField(receipt, "validation", "unavailable");
  iggy3d::appendReceiptField(receipt, "sync_validation", "unavailable");
  iggy3d::appendReceiptField(receipt, "function_loading_clean", false);
  iggy3d::appendReceiptField(receipt, "result", result);
  iggy3d::appendReceiptField(receipt, "reason_code", reason);
  return receipt;
}

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
  create.title = "iggy3d device smoke";
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

  iggy3d::VulkanBackendCreateInfo backendInfo;
  backendInfo.config.allowSoftwareVulkan = true;
  backendInfo.surfaceProvider.requiredInstanceExtensions = extensions.names;
  backendInfo.surfaceProvider.createSurface =
      [&sdlProvider, &window](VkInstance instance, VkSurfaceKHR* surface) {
        iggy3d::SdlVulkanSurfaceCreateResult created =
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
  iggy3d::RenderReceipt receipt = backend.diagnostics();
  const bool ready = backend.lifecycleState() == iggy3d::RendererLifecycleState::Ready;
  iggy3d::appendReceiptField(receipt, "smoke", "vulkan_device");
  backend.shutdown();
  if (!ready) {
    return printReceipt(baseReceipt(strictLane() ? "fail" : "skip",
                                    strictLane() ? "vulkan_smoke_strict_dependency_missing"
                                                 : "vulkan_smoke_skipped_no_suitable_device"),
                        strictLane() ? 1 : 77);
  }
  return printReceipt(receipt, 0);
#endif
}
