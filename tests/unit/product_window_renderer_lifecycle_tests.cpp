#include "app/iggy3d/ProductWindowRendererLifecycle.hpp"

#include <iostream>
#include <string_view>

#include "render/RenderDiagnostics.hpp"

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::ProductAppWindowState requestedWindow() {
  iggy3d::ProductAppWindowState window;
  window.productVulkanRendererRequested = true;
  return window;
}

bool notRequestedReportsStableBlocker() {
  const iggy3d::ProductAppWindowState window;
  const iggy3d::ProductVulkanGameplayReadiness readiness =
      iggy3d::evaluateProductVulkanGameplayReadiness(window);
  return expect(!readiness.ready, "not requested not ready") &&
         expect(readiness.backendBuilt == iggy3d::productWindowVulkanBackendBuilt(),
                "backend built reflects compile configuration") &&
         expect(readiness.status == "product_vulkan_not_requested",
                "not requested status") &&
         expect(readiness.reasonCode == "product_vulkan_not_requested",
                "not requested reason");
}

bool unavailableRendererReportsRuntimeBlockerWhenBackendBuilt() {
  iggy3d::ProductAppWindowState window = requestedWindow();
  window.productVulkanReasonCode = "renderer_unavailable";
  const iggy3d::ProductVulkanGameplayReadiness readiness =
      iggy3d::evaluateProductVulkanGameplayReadiness(window);
  if (!iggy3d::productWindowVulkanBackendBuilt()) {
    return expect(!readiness.ready, "backend-unavailable build not ready") &&
           expect(readiness.status == "product_vulkan_backend_unavailable",
                  "backend-unavailable build status") &&
           expect(readiness.reasonCode == "product_vulkan_backend_unavailable",
                  "backend-unavailable build reason");
  }
  return expect(!readiness.ready, "unavailable renderer not ready") &&
         expect(readiness.status == "product_vulkan_renderer_unavailable",
                "unavailable renderer status") &&
         expect(readiness.reasonCode == "renderer_unavailable",
                "unavailable renderer reason");
}

bool cpuMeshReadinessIsRequiredBeforeGameplayReady() {
  iggy3d::ProductAppWindowState window = requestedWindow();
  window.productVulkanRendererCreated = true;
  window.productVulkanRendererReady = true;
  const iggy3d::ProductVulkanGameplayReadiness readiness =
      iggy3d::evaluateProductVulkanGameplayReadiness(window);
  if (!iggy3d::productWindowVulkanBackendBuilt()) {
    return expect(readiness.status == "product_vulkan_backend_unavailable",
                  "cpu-ready gate blocked by unavailable backend");
  }
  return expect(!readiness.ready, "cpu mesh missing not ready") &&
         expect(readiness.status == "product_vulkan_room_mesh_cpu_not_ready",
                "cpu mesh missing status") &&
         expect(readiness.reasonCode == "product_vulkan_room_mesh_cpu_not_ready",
                "cpu mesh missing reason");
}

bool frameSubmitIsRequiredBeforeGameplayReady() {
  iggy3d::ProductAppWindowState window = requestedWindow();
  window.productVulkanRendererCreated = true;
  window.productVulkanRendererReady = true;
  window.viewport.productVulkanRoomMeshCpuReady = true;
  window.productVulkanReasonCode = "product_vulkan_waiting_for_gameplay_room";
  const iggy3d::ProductVulkanGameplayReadiness readiness =
      iggy3d::evaluateProductVulkanGameplayReadiness(window);
  if (!iggy3d::productWindowVulkanBackendBuilt()) {
    return expect(readiness.status == "product_vulkan_backend_unavailable",
                  "frame-submit gate blocked by unavailable backend");
  }
  return expect(!readiness.ready, "frame missing not ready") &&
         expect(readiness.status == "product_vulkan_frame_not_submitted",
                "frame missing status") &&
         expect(readiness.reasonCode == "product_vulkan_waiting_for_gameplay_room",
                "frame missing reason");
}

bool roomMeshBackendPresentationIsRequiredBeforeGameplayReady() {
  iggy3d::ProductAppWindowState window = requestedWindow();
  window.productVulkanRendererCreated = true;
  window.productVulkanRendererReady = true;
  window.viewport.productVulkanRoomMeshCpuReady = true;
  window.productVulkanFrameSubmitted = true;
  window.productVulkanRenderingPath = "diagnostic";
  window.productVulkanRecordMode = "none";
  const iggy3d::ProductVulkanGameplayReadiness readiness =
      iggy3d::evaluateProductVulkanGameplayReadiness(window);
  if (!iggy3d::productWindowVulkanBackendBuilt()) {
    return expect(readiness.status == "product_vulkan_backend_unavailable",
                  "room-mesh gate blocked by unavailable backend");
  }
  return expect(!readiness.ready, "room mesh backend missing not ready") &&
         expect(readiness.status == "product_vulkan_room_mesh_not_presented",
                "room mesh backend missing status") &&
         expect(readiness.reasonCode == "product_vulkan_room_mesh_not_presented",
                "room mesh backend missing reason");
}

bool roomMeshFramePathReportsGameplayReadyWhenBackendBuilt() {
  iggy3d::ProductAppWindowState window = requestedWindow();
  window.productVulkanRendererCreated = true;
  window.productVulkanRendererReady = true;
  window.viewport.productVulkanRoomMeshCpuReady = true;
  window.productVulkanFrameSubmitted = true;
  window.productVulkanRenderingPath = "package_room_meshes";
  window.productVulkanRecordMode = "room_mesh_draws";
  window.viewport.productVulkanRoomMeshBackendPresented = true;
  const iggy3d::ProductVulkanGameplayReadiness readiness =
      iggy3d::evaluateProductVulkanGameplayReadiness(window);
  if (!iggy3d::productWindowVulkanBackendBuilt()) {
    return expect(!readiness.ready, "ready case blocked when backend absent") &&
           expect(readiness.status == "product_vulkan_backend_unavailable",
                  "ready case backend absent status");
  }
  return expect(readiness.ready, "room mesh frame path ready") &&
         expect(readiness.status == "product_vulkan_gameplay_ready",
                "gameplay ready status") &&
         expect(readiness.reasonCode == "product_vulkan_gameplay_ready",
                "gameplay ready reason");
}

bool productReceiptCarriesReadinessFields() {
  iggy3d::ProductAppOptions options;
  iggy3d::ProductWorldTemplate world;
  iggy3d::FrontendState frontend;
  iggy3d::FrontendSettings settings;
  iggy3d::ProductSaveBridgeResult saves;
  iggy3d::ProductAppWindowState window = requestedWindow();
  window.productVulkanRendererCreated = true;
  window.productVulkanRendererReady = true;
  window.viewport.productVulkanRoomMeshCpuReady = true;
  window.productVulkanFrameSubmitted = true;
  window.productVulkanRenderingPath = "package_room_meshes";
  window.productVulkanRecordMode = "room_mesh_draws";
  window.viewport.productVulkanRoomMeshBackendPresented = true;
  const iggy3d::RenderReceipt receipt =
      iggy3d::buildProductAppReceipt(options, world, frontend, settings, window, saves);

  const bool expectedReady = iggy3d::productWindowVulkanBackendBuilt();
  return expect(iggy3d::hasReceiptField(
                    receipt,
                    "product_vulkan_backend_built",
                    expectedReady ? "true" : "false"),
                "receipt includes backend built") &&
         expect(iggy3d::hasReceiptField(
                    receipt,
                    "product_vulkan_gameplay_ready",
                    expectedReady ? "true" : "false"),
                "receipt includes gameplay readiness") &&
         expect(iggy3d::hasReceiptField(
                    receipt,
                    "product_vulkan_gameplay_status",
                    expectedReady ? "product_vulkan_gameplay_ready"
                                  : "product_vulkan_backend_unavailable"),
                "receipt includes gameplay readiness status") &&
         expect(iggy3d::hasReceiptField(
                    receipt,
                    "product_vulkan_gameplay_reason_code",
                    expectedReady ? "product_vulkan_gameplay_ready"
                                  : "product_vulkan_backend_unavailable"),
                "receipt includes gameplay readiness reason");
}

}  // namespace

int main() {
  const bool ok =
      notRequestedReportsStableBlocker() &&
      unavailableRendererReportsRuntimeBlockerWhenBackendBuilt() &&
      cpuMeshReadinessIsRequiredBeforeGameplayReady() &&
      frameSubmitIsRequiredBeforeGameplayReady() &&
      roomMeshBackendPresentationIsRequiredBeforeGameplayReady() &&
      roomMeshFramePathReportsGameplayReadyWhenBackendBuilt() &&
      productReceiptCarriesReadinessFields();
  if (!ok) {
    return 1;
  }
  std::cout << "product_window_renderer_lifecycle_tests=pass\n";
  return 0;
}
