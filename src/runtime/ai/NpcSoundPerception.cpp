#include "runtime/ai/NpcSoundPerception.hpp"

#include <algorithm>
#include <cmath>

namespace iggy3d {
namespace {

float distanceMeters(Vec3 a, Vec3 b) { return std::sqrt(lengthSquared(a - b)); }

}  // namespace

float soundAudibilityDb(const SoundEvent& event, Vec3 listenerPos,
                        const SoundPerceptionConfig& config, bool blockerBetween) {
  const float dist = distanceMeters(event.originMeters, listenerPos);
  float base = event.loudnessDb -
               (config.falloffCoeff * std::log10(std::max(dist, 1.0F)) +
                config.airLossDbPerMeter * dist);
  if (blockerBetween) {
    base -= config.perWallLossDb;  // apply-once
  }
  return base;
}

bool hearsSound(float audibilityDb, float hearingThresholdDb) {
  return audibilityDb > hearingThresholdDb;  // strict: at-threshold is not heard
}

float soundToAlertUnits(float audibilityDb, float hearingThresholdDb, float alertFactor,
                        float alertMax) {
  const float units = 1.0F + (audibilityDb - hearingThresholdDb) * alertFactor;
  return std::clamp(units, 0.0F, alertMax);
}

float soundAudibleRangeMeters(float loudnessDb) {
  return std::pow(2.0F, (loudnessDb - 30.0F) / 7.0F) * 2.2F;
}

SoundPerceptionResult resolveLoudestSound(std::span<const SoundEvent> events, Vec3 listenerPos,
                                          const SoundPerceptionConfig& config,
                                          std::span<const bool> blockers) {
  SoundPerceptionResult result;
  for (std::size_t i = 0; i < events.size(); ++i) {
    const SoundEvent& event = events[i];
    const bool blockerBetween = i < blockers.size() && blockers[i];
    // Cheap range-sphere reject before the audibility math.
    if (distanceMeters(event.originMeters, listenerPos) >
        soundAudibleRangeMeters(event.loudnessDb)) {
      continue;
    }
    const float audibilityDb = soundAudibilityDb(event, listenerPos, config, blockerBetween);
    if (!hearsSound(audibilityDb, config.hearingThresholdDb)) {
      continue;
    }
    // Highest-wins per tick.
    if (!result.heard || audibilityDb > result.audibilityDb) {
      result.heard = true;
      result.audibilityDb = audibilityDb;
      result.alertUnits = soundToAlertUnits(audibilityDb, config.hearingThresholdDb,
                                            event.alertFactor, event.alertMax);
      result.investigatePos = event.originMeters;  // v1: TRUE origin
    }
  }
  return result;
}

}  // namespace iggy3d
