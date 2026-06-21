#pragma once

#include <cmath>
#include <cstdint>

namespace iggy3d {

enum class RuntimeConfigStatus : std::uint8_t {
  Ok,
  InvalidTickRate,
  InvalidInteractionRange,
  InvalidMovementDistance,
  InvalidSlowTimeScale,
};

struct RuntimeConfig {
  std::uint32_t fixedTickRateHz = 20;
  float interactionRangeMeters = 1.500F;
  float movementDistanceMeters = 3.000F;
  float slowTimeScale = 0.250F;
};

inline RuntimeConfig makeDefaultRuntimeConfig() {
  return {};
}

inline RuntimeConfigStatus validateRuntimeConfig(const RuntimeConfig& config) {
  if (config.fixedTickRateHz == 0) {
    return RuntimeConfigStatus::InvalidTickRate;
  }
  if (!std::isfinite(config.interactionRangeMeters) || config.interactionRangeMeters <= 0.0F) {
    return RuntimeConfigStatus::InvalidInteractionRange;
  }
  if (!std::isfinite(config.movementDistanceMeters) || config.movementDistanceMeters <= 0.0F) {
    return RuntimeConfigStatus::InvalidMovementDistance;
  }
  if (!std::isfinite(config.slowTimeScale) || config.slowTimeScale <= 0.0F ||
      config.slowTimeScale >= 1.0F) {
    return RuntimeConfigStatus::InvalidSlowTimeScale;
  }
  return RuntimeConfigStatus::Ok;
}

}  // namespace iggy3d
