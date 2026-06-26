#include "content/PackageLoader.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/FrameInput.hpp"
#include "render/RenderDiagnostics.hpp"
#include "runtime/replay/StateHash.hpp"
#include "runtime/session/Session.hpp"

#include <filesystem>
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
  iggy3d::appendReceiptField(receipt, "smoke", "vulkan_first_room");
  iggy3d::appendReceiptField(receipt, "packet_order", "7");
  iggy3d::appendReceiptField(receipt, "backend", "vulkan");
  iggy3d::appendReceiptField(receipt, "pipeline_family", "first_room");
  iggy3d::appendReceiptField(receipt, "pipeline_variant",
                             "first_room.vertex_color.opaque.depth.backface");
  iggy3d::appendReceiptField(receipt, "shader_language", "glsl");
  iggy3d::appendReceiptField(receipt, "frame_count_requested", static_cast<std::uint64_t>(1));
  iggy3d::appendReceiptField(receipt, "frames_presented", static_cast<std::uint64_t>(0));
  iggy3d::appendReceiptField(receipt, "draw_count", static_cast<std::uint64_t>(0));
  iggy3d::appendReceiptField(receipt, "vertex_buffer_count", static_cast<std::uint64_t>(0));
  iggy3d::appendReceiptField(receipt, "index_buffer_count", static_cast<std::uint64_t>(0));
  iggy3d::appendReceiptField(receipt, "depth_enabled", false);
  iggy3d::appendReceiptField(receipt, "room_proxy_visible", "unavailable");
  iggy3d::appendReceiptField(receipt, "player_marker_visible", "unavailable");
  iggy3d::appendReceiptField(receipt, "marker_count", static_cast<std::uint64_t>(0));
  iggy3d::appendReceiptField(receipt, "first_room_visible", false);
  iggy3d::appendReceiptField(receipt, "runtime_hash_before", "none");
  iggy3d::appendReceiptField(receipt, "runtime_hash_after", "none");
  iggy3d::appendReceiptField(receipt, "validation_error_count", static_cast<std::uint64_t>(0));
  iggy3d::appendReceiptField(receipt, "sync_validation_error_count", static_cast<std::uint64_t>(0));
  iggy3d::appendReceiptField(receipt, "result", result);
  iggy3d::appendReceiptField(receipt, "reason_code", reason);
  return receipt;
}

int printReceipt(const iggy3d::RenderReceipt& receipt, int exitCode) {
  std::cout << iggy3d::formatRenderReceipt(receipt);
  return exitCode;
}

#if defined(IGGY3D_HAS_SDL3) && defined(IGGY3D_HAS_VULKAN)
bool shaderArtifactsAvailable() {
#if defined(IGGY3D_SHADER_BINARY_ROOT_VALUE)
  const std::filesystem::path root{IGGY3D_SHADER_BINARY_ROOT_VALUE};
  return std::filesystem::exists(root / "first_room.vert.spv") &&
         std::filesystem::exists(root / "first_room.frag.spv");
#else
  return false;
#endif
}

iggy3d::FrameInput validFrame(const iggy3d::SceneProjectionResult& scene,
                              const iggy3d::DebugProjectionResult& debug,
                              std::uint32_t width,
                              std::uint32_t height) {
  iggy3d::FrameInput frame;
  frame.viewport = {width, height, static_cast<float>(width) / static_cast<float>(height)};
  frame.clock = {scene.sourceTick == iggy3d::kInvalidCommandTick ? 0U : scene.sourceTick,
                 1U, 0.0F, 1.0F / 60.0F};
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

iggy3d::Result<iggy3d::Session> loadFirstRoomSession() {
  const std::filesystem::path fixture =
      std::filesystem::current_path() / "fixtures/demos/first_room/package.iggy3d.toml";
  const iggy3d::PackageLoadResult package =
      iggy3d::loadPackage(iggy3d::PackageLoadRequest{fixture.string()});
  if (package.status != iggy3d::PackageLoadStatus::Ok) {
    return {};
  }
  iggy3d::SessionCreateRequest request;
  request.packageId = package.manifest.packageId;
  request.config = package.scenario.config;
  request.seed = package.scenario;
  return iggy3d::Session::create(request);
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
  if (!shaderArtifactsAvailable()) {
    return printReceipt(baseReceipt(strictLane() ? "fail" : "skip",
                                    strictLane() ? "packet7_smoke_strict_dependency_missing"
                                                 : "shader_artifact_missing"),
                        strictLane() ? 1 : 77);
  }

  iggy3d::Result<iggy3d::Session> sessionResult = loadFirstRoomSession();
  if (sessionResult.status != iggy3d::ResultStatus::Ok) {
    return printReceipt(baseReceipt("fail", "product_package_lookup_failed"), 1);
  }
  iggy3d::Session session = std::move(sessionResult.value);
  const iggy3d::StateHashValue hashBefore = iggy3d::computeStateHash(session.state());

  iggy3d::SdlWindowCreateInfo create;
  create.title = "iggy3d first room smoke";
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
  backendInfo.config.renderer = iggy3d::RendererMode::Vulkan;
  backendInfo.config.allowSoftwareVulkan = true;
  backendInfo.config.shaderRoot = std::filesystem::path{IGGY3D_SHADER_BINARY_ROOT_VALUE};
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

  const iggy3d::SceneProjectionResult scene = iggy3d::buildSceneProjection(session.state());
  const iggy3d::DebugProjectionResult debug = iggy3d::buildDebugProjection(session.state());
  const iggy3d::RenderSubmitResult submitted =
      backend.submitFrame(validFrame(scene, debug, extent.width, extent.height));
  const iggy3d::StateHashValue hashAfter = iggy3d::computeStateHash(session.state());

  iggy3d::RenderReceipt receipt = submitted.receipt;
  iggy3d::appendReceiptField(receipt, "smoke", "vulkan_first_room");
  iggy3d::appendReceiptField(receipt, "frame_count_requested", static_cast<std::uint64_t>(1));
  iggy3d::appendReceiptField(receipt, "frames_presented",
                             submitted.outcome == iggy3d::RenderOutcome::Ok
                                 ? static_cast<std::uint64_t>(1)
                                 : static_cast<std::uint64_t>(0));
  iggy3d::appendReceiptField(receipt, "vertex_buffer_count", static_cast<std::uint64_t>(1));
  iggy3d::appendReceiptField(receipt, "index_buffer_count", static_cast<std::uint64_t>(1));
  iggy3d::appendReceiptField(receipt, "runtime_hash_before", iggy3d::formatStateHash(hashBefore));
  iggy3d::appendReceiptField(receipt, "runtime_hash_after", iggy3d::formatStateHash(hashAfter));
  const bool ok = submitted.outcome == iggy3d::RenderOutcome::Ok &&
                  iggy3d::hasReceiptField(receipt, "record_mode", "first_room") &&
                  iggy3d::hasReceiptField(receipt, "pipeline_family", "first_room") &&
                  iggy3d::hasReceiptField(
                      receipt, "pipeline_variant",
                      "first_room.vertex_color.opaque.depth.backface") &&
                  iggy3d::hasReceiptField(receipt, "draw_count", "1") &&
                  iggy3d::hasReceiptField(receipt, "first_room_visible", "true") &&
                  iggy3d::hasReceiptField(receipt, "depth_enabled", "true") &&
                  hashBefore == hashAfter;
  backend.shutdown();
  return printReceipt(ok ? receipt : baseReceipt("fail", "vulkan_smoke_receipt_invalid"),
                      ok ? 0 : 1);
#endif
}
