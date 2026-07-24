#pragma once

#include <cstdint>
#include <filesystem>

namespace iggy3d {

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

struct RendererConfig {
  ValidationMode validation = ValidationMode::Off;
  ValidationMode syncValidation = ValidationMode::Off;
  PresentModeRequest presentMode = PresentModeRequest::Auto;
  std::filesystem::path shaderRoot;
  std::filesystem::path staticMeshAssetRoot;
  std::filesystem::path diagnosticsDir;
  bool strictVulkan = false;
  bool allowSoftwareVulkan = false;
  std::uint32_t maxFramesInFlight = 2;
};

[[nodiscard]] constexpr bool isValidRendererConfig(
    const RendererConfig& config) noexcept {
  return config.maxFramesInFlight >= 1U &&
         config.maxFramesInFlight <= 3U;
}

}  // namespace iggy3d
