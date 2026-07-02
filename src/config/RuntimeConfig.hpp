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
  // Player-footstep sound emission (A1 L1). NAMED, scenario-overridable tuning so
  // no dimension is hardcoded at the emission seam. A moving player emits a
  // SoundEvent whose loudness is base + perMeter * horizontalDisplacement; guards
  // hear it through the attenuation kernel (NpcSoundPerception). The footstep is a
  // ~50 dB class event, tuned DOWN here to survive the garden's ~11 m sneak
  // geometry with the kernel's gentle 9 dB/decade falloff (the sneak invariant
  // holds with margin — see stealth_garden_tests). Exact values are A10's job.
  float footstepBaseLoudnessDb = 24.000F;
  float footstepLoudnessPerMeterDb = 0.500F;
  float footstepAlertFactor = 1.000F;
  float footstepAlertMaxUnits = 30.000F;
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
