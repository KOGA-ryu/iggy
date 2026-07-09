#include "EditorBootstrap.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <utility>

#include <SDL3/SDL.h>

#include "app/PackageRuntimeLookup.hpp"
#include "app/platform/SdlVulkanSurface.hpp"
#include "render/RendererApi.hpp"
#include "render/vulkan/FrameCapture.hpp"

namespace iggy3d_creative_app {

iggy3d::RendererConfig makeCreativeVulkanRendererConfig() {
  iggy3d::PackageLookupConfig lookupConfig;
  lookupConfig.packageMode = iggy3d::PackageMode::BuildTreeProduct;
  lookupConfig.requireShaderRoot = true;
  lookupConfig.requireGraphicsRuntime = true;
  const iggy3d::PackageLookupResult lookup =
      iggy3d::resolvePackageRuntimeLookup(lookupConfig);

  iggy3d::RendererConfig config;
  config.renderer = iggy3d::RendererMode::Vulkan;
  config.rendererRequirement = iggy3d::RendererRequirement::Optional;
  config.allowSoftwareVulkan = true;
  if (lookup.outcome == iggy3d::RenderOutcome::Ok) {
    config.shaderRoot = lookup.lookup.shaderRoot;
    config.diagnosticsDir = lookup.lookup.diagnosticsDir;
  }
  SDL_Log("iggy3d_creative: shader lookup outcome=%d shaderRoot='%s'",
          static_cast<int>(lookup.outcome),
          config.shaderRoot.generic_string().c_str());
  return config;
}

std::unique_ptr<iggy3d::VulkanBackend> createCreativeRenderer(
    iggy3d::SdlWindow& window) {
  iggy3d::SdlVulkanSurfaceProvider sdlVulkanProvider;
  const iggy3d::SdlVulkanExtensionList extensions =
      sdlVulkanProvider.requiredInstanceExtensions(window);
  if (extensions.outcome != iggy3d::RenderOutcome::Ok) {
    SDL_Log("iggy3d_creative: vulkan instance extensions unavailable (%s)",
            std::string(extensions.reason.code).c_str());
    return nullptr;
  }

  iggy3d::VulkanBackendCreateInfo backendInfo;
  backendInfo.config = makeCreativeVulkanRendererConfig();
  const iggy3d::SdlDrawableExtent drawable = window.drawableExtent();
  backendInfo.drawableWidth = drawable.width == 0U ? 1280U : drawable.width;
  backendInfo.drawableHeight = drawable.height == 0U ? 720U : drawable.height;
  backendInfo.surfaceProvider.requiredInstanceExtensions = extensions.names;
  backendInfo.surfaceProvider.createSurface =
      [&sdlVulkanProvider,
       &window](VkInstance instance, VkSurfaceKHR* surface) {
        const iggy3d::SdlVulkanSurfaceCreateResult created =
            sdlVulkanProvider.createSurface(window, instance);
        if (surface != nullptr) {
          *surface = created.surface;
        }
        iggy3d::RenderReceipt receipt;
        return receipt;
      };

  return std::make_unique<iggy3d::VulkanBackend>(std::move(backendInfo));
}

bool captureFrameToPng(iggy3d::VulkanBackend& backend,
                       const std::string& pngPath) {
  const iggy3d::RenderOutcome wait = backend.waitIdle();
  if (wait != iggy3d::RenderOutcome::Ok || !backend.frameCaptureReady()) {
    SDL_Log("iggy3d_creative: capture unavailable (wait=%d ready=%d)",
            static_cast<int>(wait), backend.frameCaptureReady() ? 1 : 0);
    return false;
  }
  const iggy3d::vulkan::NormalizedCapture capture =
      backend.readLastFrameCapture();
  const std::filesystem::path png{pngPath};
  iggy3d::vulkan::FrameCaptureArtifacts artifacts;
  artifacts.screenshotPath = png;
  artifacts.rawPath = std::filesystem::path{png}.replace_extension(".rgba");
  artifacts.metaPath = std::filesystem::path{png}.replace_extension(".meta.kv");
  artifacts.hashPath = std::filesystem::path{png}.replace_extension(".sha256");
  const iggy3d::vulkan::FrameCaptureResult result =
      iggy3d::vulkan::writePacket7CaptureArtifacts(capture, artifacts);
  SDL_Log("iggy3d_creative: capture written=%d path='%s' %ux%u coverage=%.4f",
          result.written ? 1 : 0, png.generic_string().c_str(), capture.width,
          capture.height, result.nonBackgroundPixelCoverage);
  return result.written;
}

}  // namespace iggy3d_creative_app
