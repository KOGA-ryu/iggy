#include "render/RendererConfig.hpp"

#include <utility>

namespace iggy3d {
namespace {

bool isVulkanBuilt() {
#if defined(IGGY3D_ENABLE_VULKAN) && IGGY3D_ENABLE_VULKAN
  return true;
#else
  return false;
#endif
}

bool requiresVulkanValidation(const RendererConfig& config) {
  return config.validation == ValidationMode::Required ||
         config.syncValidation == ValidationMode::Required ||
         config.debugLabels == DebugLabelsMode::Required;
}

RenderReason reasonFor(std::string_view code) {
  if (code == "renderer_config_conflict") {
    return {code, "renderer config conflict"};
  }
  if (code == "renderer_config_backend_unavailable") {
    return {code, "renderer backend unavailable"};
  }
  if (code == "renderer_config_validation_without_vulkan") {
    return {code, "renderer validation requires vulkan"};
  }
  if (code == "renderer_config_frames_in_flight_invalid") {
    return {code, "renderer frames in flight invalid"};
  }
  if (code == "renderer_config_shader_root_missing") {
    return {code, "renderer shader root missing"};
  }
  if (code == "renderer_config_diagnostics_dir_missing") {
    return {code, "renderer diagnostics directory missing"};
  }
  return {"renderer_config_ok", "renderer config ok"};
}

void appendConfigReceiptFields(RenderReceipt& receipt,
                               const RendererConfig& config,
                               std::string_view reasonCode) {
  appendReceiptField(receipt, "receipt_version", "1");
  appendReceiptField(receipt, "repo", "iggy3d");
  appendReceiptField(receipt, "file_plan", "src/render/RendererConfig.cpp");
  appendReceiptField(receipt, "packet_order", "1");
  appendReceiptField(receipt, "allowed_to_implement_code_now", "false");
  appendReceiptField(receipt, "renderer_mode", rendererModeName(config.renderer));
  appendReceiptField(receipt, "renderer_required",
                     config.rendererRequirement == RendererRequirement::Required);
  appendReceiptField(receipt, "validation_mode", validationModeName(config.validation));
  appendReceiptField(receipt, "sync_validation_mode", validationModeName(config.syncValidation));
  appendReceiptField(receipt, "debug_labels_mode", debugLabelsModeName(config.debugLabels));
  appendReceiptField(receipt, "present_mode_request", presentModeRequestName(config.presentMode));
  appendReceiptField(receipt, "shader_root",
                     config.shaderRoot.empty() ? "unavailable" : config.shaderRoot.generic_string());
  appendReceiptField(receipt, "static_mesh_asset_root",
                     config.staticMeshAssetRoot.empty()
                         ? "unavailable"
                         : config.staticMeshAssetRoot.generic_string());
  appendReceiptField(receipt, "diagnostics_dir",
                     config.diagnosticsDir.empty() ? "unavailable"
                                                   : config.diagnosticsDir.generic_string());
  appendReceiptField(receipt, "strict_vulkan", config.strictVulkan);
  appendReceiptField(receipt, "allow_software_vulkan", config.allowSoftwareVulkan);
  appendReceiptField(receipt, "max_frames_in_flight",
                     static_cast<std::uint64_t>(config.maxFramesInFlight));
  appendReceiptField(receipt, "reason_code", reasonCode);
}

RendererConfigResult makeResult(RendererConfig config,
                                RenderOutcome outcome,
                                std::string_view reasonCode) {
  RendererConfigResult result;
  result.config = std::move(config);
  result.outcome = outcome;
  result.reason = reasonFor(reasonCode);
  appendConfigReceiptFields(result.receipt, result.config, result.reason.code);
  return result;
}

}  // namespace

std::string_view rendererModeName(RendererMode mode) {
  switch (mode) {
    case RendererMode::Null:
      return "null";
    case RendererMode::Vulkan:
      return "vulkan";
    case RendererMode::Auto:
      return "auto";
  }
  return "auto";
}

std::string_view validationModeName(ValidationMode mode) {
  switch (mode) {
    case ValidationMode::Off:
      return "off";
    case ValidationMode::Optional:
      return "optional";
    case ValidationMode::Required:
      return "required";
  }
  return "off";
}

std::string_view debugLabelsModeName(DebugLabelsMode mode) {
  switch (mode) {
    case DebugLabelsMode::Off:
      return "off";
    case DebugLabelsMode::Optional:
      return "optional";
    case DebugLabelsMode::Required:
      return "required";
  }
  return "off";
}

std::string_view presentModeRequestName(PresentModeRequest mode) {
  switch (mode) {
    case PresentModeRequest::Auto:
      return "auto";
    case PresentModeRequest::Fifo:
      return "fifo";
    case PresentModeRequest::Mailbox:
      return "mailbox";
    case PresentModeRequest::Immediate:
      return "immediate";
  }
  return "auto";
}

RendererConfigResult resolveRendererConfig(const RendererConfig& input) {
  RendererConfig config = input;
  if (config.renderer == RendererMode::Auto) {
    config.renderer = isVulkanBuilt() ? RendererMode::Vulkan : RendererMode::Null;
  }

  if (config.maxFramesInFlight < 1U || config.maxFramesInFlight > 3U) {
    return makeResult(std::move(config), RenderOutcome::ValidationFailure,
                      "renderer_config_frames_in_flight_invalid");
  }

  if (config.renderer == RendererMode::Null && requiresVulkanValidation(config)) {
    return makeResult(std::move(config), RenderOutcome::ValidationFailure,
                      "renderer_config_validation_without_vulkan");
  }

  if (config.renderer == RendererMode::Null &&
      config.rendererRequirement == RendererRequirement::Required) {
    return makeResult(std::move(config), RenderOutcome::Unsupported, "renderer_config_conflict");
  }

  if (config.renderer == RendererMode::Vulkan && !isVulkanBuilt()) {
    return makeResult(std::move(config), RenderOutcome::Unsupported,
                      "renderer_config_backend_unavailable");
  }

  return makeResult(std::move(config), RenderOutcome::Ok, "renderer_config_ok");
}

}  // namespace iggy3d
