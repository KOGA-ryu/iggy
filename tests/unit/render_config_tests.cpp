#include "render/RendererConfig.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool defaultConfigResolvesDeterministically() {
  const iggy3d::RendererConfigResult result = iggy3d::resolveRendererConfig({});
  return expect(result.outcome == iggy3d::RenderOutcome::Ok, "default outcome") &&
         expect(result.reason.code == "renderer_config_ok", "default reason") &&
         expect(result.config.renderer == iggy3d::RendererMode::Null, "default null") &&
         expect(result.config.maxFramesInFlight == 2U, "default frames") &&
         expect(iggy3d::hasReceiptField(result.receipt, "renderer_mode", "null"),
                "default receipt");
}

bool backendAndRequirementConflictsAreDiagnosed() {
  iggy3d::RendererConfig vulkan;
  vulkan.renderer = iggy3d::RendererMode::Vulkan;
  const iggy3d::RendererConfigResult vulkanResult = iggy3d::resolveRendererConfig(vulkan);

  iggy3d::RendererConfig requiredNull;
  requiredNull.renderer = iggy3d::RendererMode::Null;
  requiredNull.rendererRequirement = iggy3d::RendererRequirement::Required;
  const iggy3d::RendererConfigResult requiredNullResult =
      iggy3d::resolveRendererConfig(requiredNull);

  iggy3d::RendererConfig validation;
  validation.renderer = iggy3d::RendererMode::Null;
  validation.validation = iggy3d::ValidationMode::Required;
  const iggy3d::RendererConfigResult validationResult =
      iggy3d::resolveRendererConfig(validation);

  return expect(vulkanResult.outcome == iggy3d::RenderOutcome::Unsupported, "vulkan outcome") &&
         expect(vulkanResult.reason.code == "renderer_config_backend_unavailable",
                "vulkan unavailable") &&
         expect(requiredNullResult.reason.code == "renderer_config_conflict",
                "required null conflict") &&
         expect(validationResult.reason.code == "renderer_config_validation_without_vulkan",
                "validation without backend");
}

bool framesInFlightAreBounded() {
  bool ok = true;
  for (const std::uint32_t count : {1U, 2U, 3U}) {
    iggy3d::RendererConfig config;
    config.maxFramesInFlight = count;
    ok = expect(iggy3d::resolveRendererConfig(config).outcome == iggy3d::RenderOutcome::Ok,
                "valid frame count") &&
         ok;
  }

  iggy3d::RendererConfig zero;
  zero.maxFramesInFlight = 0U;
  iggy3d::RendererConfig tooLarge;
  tooLarge.maxFramesInFlight = 4U;
  return ok &&
         expect(iggy3d::resolveRendererConfig(zero).reason.code ==
                    "renderer_config_frames_in_flight_invalid",
                "zero rejected") &&
         expect(iggy3d::resolveRendererConfig(tooLarge).reason.code ==
                    "renderer_config_frames_in_flight_invalid",
                "large rejected");
}

bool namesAndPathsStayBackendNeutral() {
  iggy3d::RendererConfig config;
  config.diagnosticsDir = "build/artifacts/render_diagnostics";
  const iggy3d::RendererConfigResult result = iggy3d::resolveRendererConfig(config);
  return expect(iggy3d::rendererModeName(iggy3d::RendererMode::Auto) == "auto", "auto name") &&
         expect(iggy3d::presentModeRequestName(iggy3d::PresentModeRequest::Fifo) == "fifo",
                "fifo name") &&
         expect(iggy3d::presentModeRequestName(iggy3d::PresentModeRequest::Mailbox) ==
                    "mailbox",
                "mailbox name") &&
         expect(iggy3d::presentModeRequestName(iggy3d::PresentModeRequest::Immediate) ==
                    "immediate",
                "immediate name") &&
         expect(result.outcome == iggy3d::RenderOutcome::Ok, "empty shader root valid") &&
         expect(result.config.shaderRoot.empty(), "shader root empty") &&
         expect(iggy3d::hasReceiptField(result.receipt, "diagnostics_dir",
                                        "build/artifacts/render_diagnostics"),
                "diagnostics dir recorded only");
}

}  // namespace

int main() {
  bool ok = true;
  ok = defaultConfigResolvesDeterministically() && ok;
  ok = backendAndRequirementConflictsAreDiagnosed() && ok;
  ok = framesInFlightAreBounded() && ok;
  ok = namesAndPathsStayBackendNeutral() && ok;
  return ok ? 0 : 1;
}
