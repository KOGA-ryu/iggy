#include "content/PackageLoader.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/FrameInput.hpp"
#include "render/RenderDiagnostics.hpp"
#include "runtime/session/Session.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

#if defined(IGGY3D_HAS_SDL3) && defined(IGGY3D_HAS_VULKAN)
#include "app/platform/SdlVulkanSurface.hpp"
#include "app/platform/SdlWindow.hpp"
#include "render/vulkan/FrameCapture.hpp"
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
  iggy3d::appendReceiptField(receipt, "smoke", "vulkan_frame_hash");
  iggy3d::appendReceiptField(receipt, "frame_hash_algorithm", "sha256_normalized_rgb");
  iggy3d::appendReceiptField(receipt, "frame_hash_exact", "none");
  iggy3d::appendReceiptField(receipt, "frame_hash_path", "");
  iggy3d::appendReceiptField(receipt, "capture_width", static_cast<std::uint64_t>(0));
  iggy3d::appendReceiptField(receipt, "capture_height", static_cast<std::uint64_t>(0));
  iggy3d::appendReceiptField(receipt, "non_background_pixel_coverage", "0.000000");
  iggy3d::appendReceiptField(receipt, "room_proxy_visible", "unavailable");
  iggy3d::appendReceiptField(receipt, "player_marker_visible", "unavailable");
  iggy3d::appendReceiptField(receipt, "visible_room_rendered", "unavailable");
  iggy3d::appendReceiptField(receipt, "exact_hash_matched", "unavailable");
  iggy3d::appendReceiptField(receipt, "tolerant_visibility_passed", false);
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
  frame.clock = {0U, 1U, 0.0F, 1.0F / 60.0F};
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

iggy3d::Result<iggy3d::Session> loadSession() {
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
  iggy3d::Result<iggy3d::Session> sessionResult = loadSession();
  if (sessionResult.status != iggy3d::ResultStatus::Ok) {
    return printReceipt(baseReceipt("fail", "visual_demo_package_lookup_failed"), 1);
  }
  iggy3d::Session session = std::move(sessionResult.value);
  iggy3d::SdlWindowCreateInfo create;
  create.title = "iggy3d frame hash smoke";
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
  if (submitted.outcome != iggy3d::RenderOutcome::Ok || !backend.frameCaptureReady()) {
    backend.shutdown();
    return printReceipt(baseReceipt(strictLane() ? "fail" : "skip",
                                    "screenshot_capture_unavailable"),
                        strictLane() ? 1 : 77);
  }
  const iggy3d::RenderOutcome waitOutcome = backend.waitIdle();
  if (waitOutcome != iggy3d::RenderOutcome::Ok) {
    backend.shutdown();
    iggy3d::RenderReceipt receipt =
        baseReceipt(strictLane() ? "fail" : "skip", "capture_gpu_wait_failed");
    iggy3d::appendReceiptField(receipt, "capture_gpu_wait", "fail");
    iggy3d::appendReceiptField(receipt, "capture_gpu_wait_reason",
                               iggy3d::renderOutcomeName(waitOutcome));
    return printReceipt(receipt, strictLane() ? 1 : 77);
  }
  const iggy3d::vulkan::NormalizedCapture capture = backend.readLastFrameCapture();
  const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy3d_packet7";
  iggy3d::vulkan::FrameCaptureArtifacts artifacts;
  artifacts.screenshotPath = root / "first_room_hash.png";
  artifacts.rawPath = root / "first_room_hash.rgba";
  artifacts.metaPath = root / "first_room_hash.meta.kv";
  artifacts.hashPath = root / "first_room_hash.sha256";
  const iggy3d::vulkan::FrameCaptureResult captureResult =
      iggy3d::vulkan::writePacket7CaptureArtifacts(capture, artifacts);
  backend.shutdown();

  const bool tolerantVisible = captureResult.nonBackgroundPixelCoverage > 0.0;
  iggy3d::RenderReceipt receipt;
  iggy3d::appendReceiptField(receipt, "smoke", "vulkan_frame_hash");
  iggy3d::appendReceiptField(receipt, "frame_hash_algorithm", "sha256_normalized_rgb");
  iggy3d::appendReceiptField(receipt, "frame_hash_exact",
                             captureResult.hash.empty() ? "none" : captureResult.hash);
  iggy3d::appendReceiptField(receipt, "frame_hash_path", artifacts.hashPath.string());
  iggy3d::appendReceiptField(receipt, "capture_gpu_wait", "pass");
  iggy3d::appendReceiptField(receipt, "capture_gpu_wait_reason", "ok");
  iggy3d::appendReceiptField(receipt, "capture_source_format", capture.sourceFormat);
  iggy3d::appendReceiptField(receipt, "capture_width",
                             static_cast<std::uint64_t>(capture.width));
  iggy3d::appendReceiptField(receipt, "capture_height",
                             static_cast<std::uint64_t>(capture.height));
  iggy3d::appendReceiptField(receipt, "non_background_pixel_coverage",
                             std::to_string(captureResult.nonBackgroundPixelCoverage));
  iggy3d::appendReceiptField(receipt, "room_proxy_visible", tolerantVisible ? "true" : "false");
  iggy3d::appendReceiptField(receipt, "player_marker_visible", "unavailable");
  iggy3d::appendReceiptField(receipt, "visible_room_rendered", tolerantVisible ? "true" : "false");
  iggy3d::appendReceiptField(receipt, "exact_hash_matched", "unavailable");
  iggy3d::appendReceiptField(receipt, "tolerant_visibility_passed", tolerantVisible);
  iggy3d::appendReceiptField(receipt, "result",
                             captureResult.written && tolerantVisible ? "pass" : "fail");
  iggy3d::appendReceiptField(receipt, "reason_code",
                             captureResult.written && tolerantVisible
                                 ? "packet7_frame_hash_written"
                                 : "vulkan_smoke_receipt_invalid");
  return printReceipt(receipt, captureResult.written && tolerantVisible ? 0 : 1);
#endif
}
