#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>

#include "render/RenderDiagnostics.hpp"

namespace iggy3d {

enum class RendererMode : std::uint8_t {
  Null,
  Vulkan,
  Auto,
};

enum class RendererRequirement : std::uint8_t {
  Optional,
  Required,
};

enum class ValidationMode : std::uint8_t {
  Off,
  Optional,
  Required,
};

enum class PresentModeRequest : std::uint8_t {
  Auto,
  Fifo,
  Mailbox,
  Immediate,
};

enum class DebugLabelsMode : std::uint8_t {
  Off,
  Optional,
  Required,
};

struct RendererConfig {
  RendererMode renderer = RendererMode::Null;
  RendererRequirement rendererRequirement = RendererRequirement::Optional;
  ValidationMode validation = ValidationMode::Off;
  ValidationMode syncValidation = ValidationMode::Off;
  DebugLabelsMode debugLabels = DebugLabelsMode::Off;
  PresentModeRequest presentMode = PresentModeRequest::Auto;
  std::filesystem::path shaderRoot;
  std::filesystem::path diagnosticsDir;
  bool strictVulkan = false;
  bool allowSoftwareVulkan = false;
  std::uint32_t maxFramesInFlight = 2;
};

struct RendererConfigResult {
  RendererConfig config;
  RenderOutcome outcome = RenderOutcome::Ok;
  RenderReason reason{"renderer_config_ok", "renderer config ok"};
  RenderReceipt receipt;
};

RendererConfigResult resolveRendererConfig(const RendererConfig& input);
std::string_view rendererModeName(RendererMode mode);
std::string_view rendererRequirementName(RendererRequirement requirement);
std::string_view validationModeName(ValidationMode mode);
std::string_view debugLabelsModeName(DebugLabelsMode mode);
std::string_view presentModeRequestName(PresentModeRequest mode);

}  // namespace iggy3d
