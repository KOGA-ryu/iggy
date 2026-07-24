#include "render/RendererConfig.hpp"
#include "render/vulkan/VulkanBackend.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>
#include <utility>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool defaultsDescribeAValidVulkanConfiguration() {
  const iggy3d::RendererConfig config;
  return expect(iggy3d::isValidRendererConfig(config),
                "default renderer config valid") &&
         expect(config.validation == iggy3d::ValidationMode::Off,
                "validation defaults off") &&
         expect(config.syncValidation == iggy3d::ValidationMode::Off,
                "sync validation defaults off") &&
         expect(config.presentMode == iggy3d::PresentModeRequest::Auto,
                "present mode defaults automatic") &&
         expect(config.maxFramesInFlight == 2U,
                "frame slot count defaults to two");
}

bool frameSlotsMatchTheLiveBackendBound() {
  bool ok = true;
  for (const std::uint32_t count : {1U, 2U, 3U}) {
    iggy3d::RendererConfig config;
    config.maxFramesInFlight = count;
    ok = expect(iggy3d::isValidRendererConfig(config),
                "supported frame slot count accepted") &&
         ok;
  }

  iggy3d::RendererConfig zero;
  zero.maxFramesInFlight = 0U;
  iggy3d::RendererConfig tooLarge;
  tooLarge.maxFramesInFlight = 4U;
  return ok &&
         expect(!iggy3d::isValidRendererConfig(zero),
                "zero frame slots rejected") &&
         expect(!iggy3d::isValidRendererConfig(tooLarge),
                "more than three frame slots rejected");
}

bool liveVulkanOptionsRemainIndependent() {
  iggy3d::RendererConfig config;
  config.validation = iggy3d::ValidationMode::Required;
  config.syncValidation = iggy3d::ValidationMode::Optional;
  config.presentMode = iggy3d::PresentModeRequest::Mailbox;
  config.shaderRoot = "build/shaders";
  config.staticMeshAssetRoot = "assets/creative";
  config.diagnosticsDir = "build/artifacts/render_diagnostics";
  config.strictVulkan = true;
  config.allowSoftwareVulkan = true;

  return expect(iggy3d::isValidRendererConfig(config),
                "non-slot Vulkan options do not invalidate config") &&
         expect(config.validation == iggy3d::ValidationMode::Required,
                "validation requirement retained") &&
         expect(config.syncValidation == iggy3d::ValidationMode::Optional,
                "sync validation requirement retained") &&
         expect(config.presentMode == iggy3d::PresentModeRequest::Mailbox,
                "present mode retained") &&
         expect(config.shaderRoot == "build/shaders",
                "shader root retained") &&
         expect(config.staticMeshAssetRoot == "assets/creative",
                "asset root retained") &&
         expect(config.diagnosticsDir ==
                    "build/artifacts/render_diagnostics",
                "diagnostics root retained") &&
         expect(config.strictVulkan && config.allowSoftwareVulkan,
                "Vulkan policy flags retained");
}

bool invalidFrameSlotsStopBeforeVulkanBootstrap() {
  iggy3d::VulkanBackendCreateInfo createInfo;
  createInfo.config.maxFramesInFlight = 0U;
  const iggy3d::VulkanBackend backend{std::move(createInfo)};

  return expect(
             backend.lifecycleState() ==
                 iggy3d::RendererLifecycleState::NotInitialized,
             "invalid config leaves Vulkan backend uninitialized") &&
         expect(iggy3d::hasReceiptField(
                    backend.diagnostics(), "reason_code",
                    "renderer_config_frames_in_flight_invalid"),
                "invalid config reports the production rejection reason");
}

}  // namespace

int main() {
  const bool ok = defaultsDescribeAValidVulkanConfiguration() &&
                  frameSlotsMatchTheLiveBackendBound() &&
                  liveVulkanOptionsRemainIndependent() &&
                  invalidFrameSlotsStopBeforeVulkanBootstrap();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
