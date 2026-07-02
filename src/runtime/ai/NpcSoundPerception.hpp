#pragma once

#include <cstddef>
#include <span>

#include "core/ids/EntityId.hpp"
#include "core/math/Vec3.hpp"

namespace iggy3d {

// Sound-perception kernel (A1 slice 1 of 2 — L1, map v1.1). Pure types + free functions, no
// state — clean-room from The Dark Mod as reference values only (stealth-ai-plan.md slice 4).
// This slice is INERT: it builds the hearing math and reserves the stimulus-bus fields but wires
// no producer and changes no behavior. a1s2 turns it on. All losses are POSITIVE (subtracted).

// A single sound stimulus. `alertFactor`/`alertMax` scale audibility -> alert units.
struct SoundEvent {
  EntityId source;
  Vec3 originMeters;
  float loudnessDb = 0.0F;
  float alertFactor = 1.0F;
  float alertMax = 30.0F;
};

// Named hearing tuning (no magic literals). Reference values seeded from TDM.
struct SoundPerceptionConfig {
  float hearingThresholdDb = 20.0F;
  float falloffCoeff = 9.0F;      // dB lost per decade of distance (9*log10(dist))
  float perWallLossDb = 20.0F;    // applied ONCE when a blocker sits between source and listener
  float airLossDbPerMeter = 0.0F; // linear air absorption (0 = none in v1)
};

struct SoundPerceptionResult {
  bool heard = false;
  float audibilityDb = 0.0F;
  float alertUnits = 0.0F;
  Vec3 investigatePos;  // v1: the sound's TRUE origin (no portal wavefront)
};

// Perceived level at the listener: loudness minus distance falloff and air loss, minus one wall
// loss when `blockerBetween` (APPLY-ONCE, never x count). Distance is floored at 1 m so the
// log10 stays finite/non-negative at point blank.
float soundAudibilityDb(const SoundEvent& event, Vec3 listenerPos,
                        const SoundPerceptionConfig& config, bool blockerBetween);

// STRICT greater-than: audibility exactly at the threshold is NOT heard (a test locks the edge).
bool hearsSound(float audibilityDb, float hearingThresholdDb);

// Map audibility above threshold to alert units, clamped to [0, alertMax].
float soundToAlertUnits(float audibilityDb, float hearingThresholdDb, float alertFactor,
                        float alertMax);

// Cheap range-sphere used to early-out before the audibility math: reference
// range = 2^((loudnessDb-30)/7) * 2.2 m.
float soundAudibleRangeMeters(float loudnessDb);

// Highest-wins reduction over one tick's events: the loudest HEARD event (after the range reject
// and threshold gate) provides audibility/alertUnits/investigatePos; `heard` is true if any was
// heard. `blockers[i]` is the apply-once wall flag for `events[i]` (absent -> no blocker).
// Additive-across-ticks is the FSM's job (a1s2), not here.
SoundPerceptionResult resolveLoudestSound(std::span<const SoundEvent> events, Vec3 listenerPos,
                                          const SoundPerceptionConfig& config,
                                          std::span<const bool> blockers);

}  // namespace iggy3d
