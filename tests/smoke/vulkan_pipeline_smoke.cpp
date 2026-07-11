#include "render/RenderDiagnostics.hpp"

#include <filesystem>
#include <iostream>
#include <string_view>

#if defined(IGGY3D_HAS_SDL3) && defined(IGGY3D_HAS_VULKAN) && \
    defined(IGGY3D_SHADER_COMPILER_AVAILABLE)
#include "app/platform/SdlVulkanSurface.hpp"
#include "app/platform/SdlWindow.hpp"
#include "render/vulkan/FirstRoomPipeline.hpp"
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
  iggy3d::appendReceiptField(receipt, "smoke", "vulkan_pipeline");
  iggy3d::appendReceiptField(receipt, "packet_order", "6");
  iggy3d::appendReceiptField(receipt, "backend", "vulkan");
  iggy3d::appendReceiptField(receipt, "shader_language", "glsl");
  iggy3d::appendReceiptField(receipt, "shader_compiler",
#if defined(IGGY3D_SHADER_COMPILER_AVAILABLE)
                             "available"
#else
                             "unavailable"
#endif
  );
  iggy3d::appendReceiptField(receipt, "shader_target_env", IGGY3D_SHADER_TARGET_ENV_VALUE);
  iggy3d::appendReceiptField(receipt, "shader_source_root", IGGY3D_SHADER_SOURCE_ROOT_VALUE);
  iggy3d::appendReceiptField(receipt, "shader_binary_root", IGGY3D_SHADER_BINARY_ROOT_VALUE);
  iggy3d::appendReceiptField(receipt, "pipeline_family", "first_room");
  iggy3d::appendReceiptField(receipt, "pipeline_created", false);
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
#elif !defined(IGGY3D_SHADER_COMPILER_AVAILABLE)
  return printReceipt(baseReceipt(strictLane() ? "fail" : "skip",
                                  strictLane() ? "packet6_smoke_strict_dependency_missing"
                                               : "packet6_smoke_skipped_shader_compiler_missing"),
                      strictLane() ? 1 : 77);
#elif !defined(IGGY3D_HAS_SDL3)
  return printReceipt(baseReceipt(strictLane() ? "fail" : "skip",
                                  strictLane() ? "packet6_smoke_strict_dependency_missing"
                                               : "vulkan_smoke_skipped_sdl_missing"),
                      strictLane() ? 1 : 77);
#else
  const std::filesystem::path shaderRoot{IGGY3D_SHADER_BINARY_ROOT_VALUE};
  const std::filesystem::path vertexPath = shaderRoot / "first_room.vert.spv";
  const std::filesystem::path fragmentPath = shaderRoot / "first_room.frag.spv";
  if (!std::filesystem::exists(vertexPath) || !std::filesystem::exists(fragmentPath)) {
    return printReceipt(baseReceipt("fail", "shader_artifact_missing"), 1);
  }

  iggy3d::SdlWindowCreateInfo create;
  create.title = "iggy3d pipeline smoke";
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
  bootstrapInfo.config.renderer = iggy3d::RendererMode::Vulkan;
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

  const iggy3d::vulkan::ShaderModuleResult vertex =
      iggy3d::vulkan::createShaderModule({bootstrap.handles().device, vertexPath,
                                          iggy3d::vulkan::ShaderStage::Vertex, "main",
                                          "shader.first_room.vertex"});
  const iggy3d::vulkan::ShaderModuleResult fragment =
      iggy3d::vulkan::createShaderModule({bootstrap.handles().device, fragmentPath,
                                          iggy3d::vulkan::ShaderStage::Fragment, "main",
                                          "shader.first_room.fragment"});
  iggy3d::vulkan::PipelineLayoutResult layout =
      iggy3d::vulkan::createFirstRoomPipelineLayout({bootstrap.handles().device, {}});
  iggy3d::vulkan::FirstRoomPipelineResult pipeline =
      iggy3d::vulkan::createFirstRoomPipeline({bootstrap.handles().device,
                                               VK_FORMAT_B8G8R8A8_SRGB,
                                               VK_FORMAT_D32_SFLOAT,
                                               vertex.record,
                                               fragment.record,
                                               layout.record});
  iggy3d::RenderReceipt receipt = pipeline.receipt;
  iggy3d::appendReceiptField(receipt, "smoke", "vulkan_pipeline");
  iggy3d::vulkan::destroyFirstRoomPipeline(bootstrap.handles().device, pipeline.record);
  iggy3d::vulkan::destroyPipelineLayout(bootstrap.handles().device, layout.record);
  iggy3d::vulkan::ShaderModuleRecord vertexRecord = vertex.record;
  iggy3d::vulkan::ShaderModuleRecord fragmentRecord = fragment.record;
  iggy3d::vulkan::destroyShaderModule(bootstrap.handles().device, vertexRecord);
  iggy3d::vulkan::destroyShaderModule(bootstrap.handles().device, fragmentRecord);
  bootstrap.shutdown();
  return printReceipt(receipt, pipeline.outcome == iggy3d::RenderOutcome::Ok ? 0 : 1);
#endif
}
