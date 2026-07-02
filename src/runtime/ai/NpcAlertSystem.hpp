#pragma once

#include <cstdint>

#include "runtime/ai/AiState.hpp"

namespace iggy3d {

// Graded-alert FSM (slice 5 of docs/stealth-ai-plan.md). Clean-room design
// derived from The Dark Mod as state-machine shape + reference values only.
//
// One scalar `alertLevel` (0..1, combat = 1.0) is raised by perception and
// decayed over time; the actor's behavior band is always DERIVED from it. All
// tuning is named data on AlertProfile (no magic literals), tick-based so every
// step is deterministic and headless-unit-testable.

// Named, normalized tuning for the graded alert ladder. Reference values seeded
// from TDM (thresholds 1.5/6/10/18/23 normalized by combat=23; drains 5/8/25/65s
// and dead-time 300ms at 20 Hz).
struct AlertProfile {
  // Ascending band boundaries on the 0..1 alert scale.
  float observantNorm = 0.065F;
  float suspiciousNorm = 0.26F;
  float searchingNorm = 0.43F;
  float agitatedNorm = 0.78F;
  float combatNorm = 1.0F;
  // Alert gained per tick while the target is perceived, scaled by proximity01.
  float riseRatePerTick = 0.05F;
  // Per-band linear decay durations in ticks: the level drops one band width
  // over drainTicks[band]. Bands (ascending): observant, suspicious, searching,
  // agitated. At 20 Hz these are ~5/8/25/65 s.
  std::uint32_t drainTicks[4] = {100U, 160U, 500U, 1300U};
  // Hold the level flat this many ticks after any rise before decay resumes.
  std::uint32_t deadTimeTicks = 6U;
  // Up-hysteresis: small non-visual rises inside the grace window are swallowed
  // (anti-spam). A LOS-confirmed sighting always bypasses this.
  float graceFrac = 1.2F;
  std::uint32_t graceWindowTicks = 40U;
  std::uint32_t graceCountLimit = 5U;
};

// Per-tick perception summary fed to the FSM, computed from an
// NpcPerceptionResult at the call site. Kept plain so the FSM stays pure.
struct NpcAlertStimulus {
  bool targetPerceived = false;   // currently visible (radius + cone + LOS)
  float proximity01 = 0.0F;       // clamp01(1 - distance/effectiveRadius)
  bool hasValidTarget = false;    // a resolved, live target entity exists
  bool visualConfirmed = false;   // LOS-confirmed sighting -> bypasses grace
};

bool isValidAlertProfile(const AlertProfile& profile);

// Band [0..5] the level falls in: 0 Idle, 1 Observant, 2 Suspicious,
// 3 Searching, 4 Alert(agitated), 5 Combat. Single source of truth is the float.
std::uint8_t alertBandIndex(float level, const AlertProfile& profile);

// Behavior derived from an alert level. Never set behavior directly.
AiBehaviorKind alertBehaviorForLevel(float level, const AlertProfile& profile);

// Decay the level toward 0: held flat for deadTimeTicks after a rise, then
// linear per-band. Clears the engagement high-water mark when fully calm.
void npcDecayAlert(AiActorState& actor, const AlertProfile& profile,
                   std::uint64_t tick);

// Raise the level by `increment`. Enforces the no-target combat cap (you can't
// fight an unnamed threat) and the grace-window anti-spam; visual sightings
// bypass grace. Resets the dead-time anchor and tracks the high-water band.
void npcRaiseAlert(AiActorState& actor, const AlertProfile& profile,
                   float increment, std::uint64_t tick, bool hasValidTarget,
                   bool visualConfirmed);

// One FSM step: rise on perception (proportional to proximity) or decay when
// nothing is perceived, then re-derive actor.behavior from the level.
void npcStepAlert(AiActorState& actor, const NpcAlertStimulus& stimulus,
                  const AlertProfile& profile, std::uint64_t tick);

}  // namespace iggy3d
