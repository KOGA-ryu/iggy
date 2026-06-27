#include "app/iggy3d/ProductWindowRendererLifecycle.hpp"

#include <memory>
#include <string>

#include "app/PackageRuntimeLookup.hpp"
#include "render/RenderDiagnostics.hpp"

#if defined(IGGY3D_HAS_SDL3)
#include <SDL3/SDL.h>
#endif

#if defined(IGGY3D_HAS_SDL3) && defined(IGGY3D_APP_VULKAN_BACKEND)
#include "app/platform/SdlVulkanSurface.hpp"
#include "render/vulkan/VulkanBackend.hpp"
#endif

namespace iggy3d {

namespace {

std::string receiptFieldValueOr(const RenderReceipt& receipt,
                                std::string_view key,
                                std::string_view fallback) {
  // branch-gate: BG-1028
  for (const RenderReceiptField& field : receipt.fields) {
    // branch-gate: BG-1028
    if (field.key == key) {
      return field.value;
    }
  }
  return std::string(fallback);
}

RendererConfig makeProductVulkanRendererConfig() {
  PackageLookupConfig lookupConfig;
  lookupConfig.packageMode = PackageMode::BuildTreeProduct;
  lookupConfig.requireShaderRoot = true;
  lookupConfig.requireGraphicsRuntime = true;
  const PackageLookupResult lookup = resolvePackageRuntimeLookup(lookupConfig);

  RendererConfig config;
  config.renderer = RendererMode::Vulkan;
  config.rendererRequirement = RendererRequirement::Optional;
  config.allowSoftwareVulkan = true;
  // branch-gate: BG-1028
  if (lookup.outcome == RenderOutcome::Ok) {
    config.shaderRoot = lookup.lookup.shaderRoot;
    config.diagnosticsDir = lookup.lookup.diagnosticsDir;
  }
  return config;
}

}  // namespace

bool productWindowRendererUsesVulkan(ProductRendererRequest rendererRequest) {
  return rendererRequest == ProductRendererRequest::Vulkan;
}

bool productWindowVulkanBackendBuilt() {
#if defined(IGGY3D_HAS_SDL3) && defined(IGGY3D_APP_VULKAN_BACKEND)
  return true;
#else
  return false;
#endif
}

ProductVulkanGameplayReadiness evaluateProductVulkanGameplayReadiness(
    const ProductAppWindowState& window) {
  ProductVulkanGameplayReadiness readiness;
  readiness.backendBuilt = productWindowVulkanBackendBuilt();
  // branch-gate: BG-1069
  if (!window.productVulkanRendererRequested) {
    readiness.status = "product_vulkan_not_requested";
    readiness.reasonCode = "product_vulkan_not_requested";
    return readiness;
  }
  // branch-gate: BG-1069
  if (!readiness.backendBuilt) {
    readiness.status = "product_vulkan_backend_unavailable";
    readiness.reasonCode = "product_vulkan_backend_unavailable";
    return readiness;
  }
  // branch-gate: BG-1069
  if (!window.productVulkanRendererCreated || !window.productVulkanRendererReady) {
    readiness.status = "product_vulkan_renderer_unavailable";
    readiness.reasonCode = window.productVulkanReasonCode.empty()
                               ? "renderer_unavailable"
                               : window.productVulkanReasonCode;
    return readiness;
  }
  // branch-gate: BG-1069
  if (!window.viewport.productVulkanRoomMeshCpuReady) {
    readiness.status = "product_vulkan_room_mesh_cpu_not_ready";
    readiness.reasonCode = "product_vulkan_room_mesh_cpu_not_ready";
    return readiness;
  }
  // branch-gate: BG-1069
  if (!window.productVulkanFrameSubmitted) {
    readiness.status = "product_vulkan_frame_not_submitted";
    readiness.reasonCode = window.productVulkanReasonCode.empty()
                               ? "frame_not_submitted"
                               : window.productVulkanReasonCode;
    return readiness;
  }
  // branch-gate: BG-1069
  if (!window.viewport.productVulkanRoomMeshBackendPresented) {
    readiness.status = "product_vulkan_room_mesh_not_presented";
    readiness.reasonCode = "product_vulkan_room_mesh_not_presented";
    return readiness;
  }
  readiness.ready = true;
  readiness.status = "product_vulkan_gameplay_ready";
  readiness.reasonCode = "product_vulkan_gameplay_ready";
  return readiness;
}

void recordProductVulkanRendererUnavailable(ProductAppWindowState& window,
                                            std::string_view reasonCode) {
  window.productVulkanRendererCreated = false;
  window.productVulkanRendererReady = false;
  window.productVulkanSurfaceCreated = false;
  window.productVulkanSwapchainReady = false;
  window.productVulkanStatus = "renderer_unavailable";
  window.productVulkanReasonCode = std::string(reasonCode);
}

void recordProductVulkanRendererReady(ProductAppWindowState& window,
                                      const RendererApi& renderer) {
  const RenderReceipt diagnostics = renderer.diagnostics();
  window.productVulkanRendererCreated = renderer.hasBackend();
  window.productVulkanRendererReady =
      renderer.lifecycleState() == RendererLifecycleState::Ready;
  window.productVulkanSurfaceCreated =
      window.productVulkanRendererReady ||
      hasReceiptField(diagnostics, "surface_ready", "true");
  window.productVulkanSwapchainReady =
      window.productVulkanRendererReady ||
      receiptFieldValueOr(diagnostics, "swapchain_state", "none") == "ready";
  window.productVulkanStatus = window.productVulkanRendererReady
                                   ? "renderer_ready"
                                   : "renderer_unavailable";
  window.productVulkanReasonCode =
      receiptFieldValueOr(diagnostics, "reason_code", "renderer_unavailable");
}

void recordProductVulkanSubmit(ProductAppWindowState& window,
                               const RenderSubmitResult& submit) {
  window.productVulkanRenderingPath =
      receiptFieldValueOr(submit.receipt, "rendering_path", "none");
  window.productVulkanRecordMode =
      receiptFieldValueOr(submit.receipt, "record_mode", "none");
  window.productVulkanReasonCode = std::string(submit.reason.code);
  // branch-gate: BG-1028
  if (submit.outcome == RenderOutcome::Ok) {
    window.productVulkanSurfaceCreated = true;
    window.productVulkanSwapchainReady = true;
    window.productVulkanFrameSubmitted = true;
    ++window.productVulkanFrameSubmittedCount;
    window.productVulkanStatus = "frame_submitted";
    // branch-gate: BG-1028
    if (window.productVulkanRenderingPath == "package_room_meshes" &&
        window.productVulkanRecordMode == "room_mesh_draws") {
      window.viewport.productVulkanRoomMeshBackendPresented = true;
    }
  } else {
    window.productVulkanStatus = "frame_not_submitted";
  }
}

ProductWindowRendererState createProductWindowRenderer(
    const ProductWindowRendererRequest& request) {
  ProductWindowRendererState renderer;
  renderer.useVulkanRenderer =
      productWindowRendererUsesVulkan(request.rendererRequest);
  // branch-gate: BG-1028
  if (request.window == nullptr || request.createInfo == nullptr ||
      request.sdlWindow == nullptr) {
    return renderer;
  }
  ProductAppWindowState& window = *request.window;

  // branch-gate: BG-1028
  if (renderer.useVulkanRenderer) {
#if defined(IGGY3D_HAS_SDL3) && defined(IGGY3D_APP_VULKAN_BACKEND)
    SdlVulkanSurfaceProvider sdlVulkanProvider;
    const SdlVulkanExtensionList extensions =
        sdlVulkanProvider.requiredInstanceExtensions(*request.sdlWindow);
    // branch-gate: BG-1028
    if (extensions.outcome != RenderOutcome::Ok) {
      recordProductVulkanRendererUnavailable(window, extensions.reason.code);
      window.status = "product_vulkan_renderer_unavailable";
      return renderer;
    }
    VulkanBackendCreateInfo backendInfo;
    backendInfo.config = makeProductVulkanRendererConfig();
    const SdlDrawableExtent drawableExtent = request.sdlWindow->drawableExtent();
    // branch-gate: BG-1028
    backendInfo.drawableWidth =
        drawableExtent.width == 0U ? request.createInfo->width : drawableExtent.width;
    // branch-gate: BG-1028
    backendInfo.drawableHeight =
        drawableExtent.height == 0U ? request.createInfo->height : drawableExtent.height;
    backendInfo.surfaceProvider.requiredInstanceExtensions = extensions.names;
    backendInfo.surfaceProvider.createSurface =
        [&sdlVulkanProvider, sdlWindow = request.sdlWindow](
            VkInstance instance, VkSurfaceKHR* surface) {
          const SdlVulkanSurfaceCreateResult created =
              sdlVulkanProvider.createSurface(*sdlWindow, instance);
          // branch-gate: BG-1028
          if (surface != nullptr) {
            *surface = created.surface;
          }
          RenderReceipt receipt;
          appendReceiptField(receipt, "surface_provider", "sdl3");
          appendReceiptField(receipt, "surface_created",
                             created.outcome == RenderOutcome::Ok &&
                                 created.surface != VK_NULL_HANDLE);
          // branch-gate: BG-1028
          appendReceiptField(receipt, "result",
                             created.outcome == RenderOutcome::Ok ? "pass" : "fail");
          appendReceiptField(receipt, "reason_code", created.reason.code);
          return receipt;
        };
    renderer.vulkanRenderer =
        RendererApi(std::make_unique<VulkanBackend>(std::move(backendInfo)));
    recordProductVulkanRendererReady(window, renderer.vulkanRenderer);
    // branch-gate: BG-1028
    if (!window.productVulkanRendererReady) {
      window.status = "product_vulkan_renderer_unavailable";
      return renderer;
    }
    renderer.ready = true;
#else
    recordProductVulkanRendererUnavailable(window, "product_vulkan_backend_unavailable");
    window.status = "product_vulkan_renderer_unavailable";
#endif
    return renderer;
  }

#if defined(IGGY3D_HAS_SDL3)
  renderer.sdlRenderer = SDL_CreateRenderer(request.sdlWindow->nativeWindow(), nullptr);
  // branch-gate: BG-1028
  if (renderer.sdlRenderer == nullptr) {
    window.status = "renderer_create_failed";
    return renderer;
  }
  renderer.ready = true;
#endif
  return renderer;
}

void shutdownProductWindowRenderer(ProductWindowRendererState& renderer) {
  // branch-gate: BG-1028
  if (renderer.useVulkanRenderer) {
    (void)renderer.vulkanRenderer.waitIdle();
    renderer.vulkanRenderer.shutdown();
  } else {
#if defined(IGGY3D_HAS_SDL3)
    SDL_DestroyRenderer(renderer.sdlRenderer);
    renderer.sdlRenderer = nullptr;
#endif
  }
  renderer.ready = false;
}

void finalizeProductWindowRendererStatus(const ProductWindowRendererState& renderer,
                                         ProductAppWindowState& window) {
  // branch-gate: BG-1028
  if (renderer.useVulkanRenderer) {
    window.status = window.productVulkanFrameSubmitted
                        ? "product_vulkan_frame_presented"
                        : window.productVulkanStatus;
  } else {
    window.status = window.menuTextDrawn ? "opening_menu_text_ready"
                                         : "opening_menu_window_ready";
  }
}

}  // namespace iggy3d
