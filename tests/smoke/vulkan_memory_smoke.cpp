#include "render/RenderDiagnostics.hpp"

#include <iostream>
#include <string_view>

#if defined(IGGY3D_HAS_SDL3) && defined(IGGY3D_HAS_VULKAN)
#include "app/platform/SdlVulkanSurface.hpp"
#include "app/platform/SdlWindow.hpp"
#include "render/vulkan/BufferImageResources.hpp"
#include "render/vulkan/InstanceDeviceSurface.hpp"
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
  iggy3d::appendReceiptField(receipt, "smoke", "vulkan_memory");
  iggy3d::appendReceiptField(receipt, "packet_order", "6");
  iggy3d::appendReceiptField(receipt, "backend", "vulkan");
  iggy3d::appendReceiptField(receipt, "memory_allocator", "manual_packet6_bootstrap");
  iggy3d::appendReceiptField(receipt, "allocation_count", static_cast<std::uint64_t>(0));
  iggy3d::appendReceiptField(receipt, "per_frame_allocation_count", static_cast<std::uint64_t>(0));
  iggy3d::appendReceiptField(receipt, "result", result);
  iggy3d::appendReceiptField(receipt, "reason_code", reason);
  return receipt;
}

int printReceipt(const iggy3d::RenderReceipt& receipt, int exitCode) {
  std::cout << iggy3d::formatRenderReceipt(receipt);
  return exitCode;
}

}  // namespace

int main() {
#if !defined(IGGY3D_HAS_VULKAN)
  return printReceipt(baseReceipt(strictLane() ? "fail" : "skip",
                                  strictLane() ? "packet6_smoke_strict_dependency_missing"
                                               : "vulkan_smoke_skipped_loader_missing"),
                      strictLane() ? 1 : 77);
#elif !defined(IGGY3D_HAS_SDL3)
  return printReceipt(baseReceipt(strictLane() ? "fail" : "skip",
                                  strictLane() ? "packet6_smoke_strict_dependency_missing"
                                               : "vulkan_smoke_skipped_sdl_missing"),
                      strictLane() ? 1 : 77);
#else
  iggy3d::SdlWindowCreateInfo create;
  create.title = "iggy3d memory smoke";
  create.width = 640U;
  create.height = 360U;
  create.vulkan = true;
  iggy3d::SdlWindow window(create);
  window.pollEvents();
  if (!window.isOpen() || !window.isDrawable()) {
    return printReceipt(baseReceipt(strictLane() ? "fail" : "skip",
                                    strictLane() ? "packet6_smoke_strict_dependency_missing"
                                                 : "vulkan_smoke_skipped_no_display"),
                        strictLane() ? 1 : 77);
  }
  iggy3d::SdlVulkanSurfaceProvider sdlProvider;
  const iggy3d::SdlVulkanExtensionList extensions =
      sdlProvider.requiredInstanceExtensions(window);
  if (extensions.outcome != iggy3d::RenderOutcome::Ok) {
    return printReceipt(baseReceipt(strictLane() ? "fail" : "skip",
                                    strictLane() ? "packet6_smoke_strict_dependency_missing"
                                                 : "vulkan_smoke_skipped_no_display"),
                        strictLane() ? 1 : 77);
  }
  iggy3d::vulkan::InstanceDeviceSurfaceCreateInfo bootstrapInfo;
  bootstrapInfo.config.allowSoftwareVulkan = true;
  bootstrapInfo.featureRequest.allowSoftwareDevice = true;
  bootstrapInfo.surfaceProvider.requiredInstanceExtensions = extensions.names;
  bootstrapInfo.surfaceProvider.createSurface =
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
  iggy3d::vulkan::InstanceDeviceSurface bootstrap;
  bootstrap.initialize(bootstrapInfo);
  if (!bootstrap.ready()) {
    return printReceipt(baseReceipt(strictLane() ? "fail" : "skip",
                                    strictLane() ? "packet6_smoke_strict_dependency_missing"
                                                 : "vulkan_smoke_skipped_no_suitable_device"),
                        strictLane() ? 1 : 77);
  }
  iggy3d::vulkan::BufferImageResources resources;
  iggy3d::vulkan::BufferImageResourcesCreateInfo resourceInfo;
  resourceInfo.physicalDevice = bootstrap.handles().physicalDevice;
  resourceInfo.device = bootstrap.handles().device;
  resourceInfo.graphicsQueue = bootstrap.handles().graphicsQueue;
  resourceInfo.graphicsQueueFamily = bootstrap.queues().graphicsFamily;
  resourceInfo.extent = {640U, 360U};
  const iggy3d::vulkan::BufferImageResourcesResult created =
      resources.createFirstRoomResources(resourceInfo);
  iggy3d::RenderReceipt receipt = created.receipt;
  iggy3d::appendReceiptField(receipt, "smoke", "vulkan_memory");
  resources.destroy();
  bootstrap.shutdown();
  return printReceipt(receipt, created.outcome == iggy3d::RenderOutcome::Ok ? 0 : 1);
#endif
}
