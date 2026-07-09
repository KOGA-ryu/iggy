#pragma once

#include <cstdint>

#include "core/math/Vec3.hpp"
#include "runtime/ai/AiState.hpp"

namespace iggy3d {

// Last-known-position memory (slice 7 of docs/stealth-ai-plan.md). Pure free functions in the
// NpcPatrolSystem/NpcAlertSystem style: while a guard visually confirms a target it remembers
// where it last saw it; when confirmation is lost but the guard is still alert (Searching band)
// it walks to that spot, looks around for a bounded dwell, and — finding nothing — gives up so
// the alert FSM decays it back to patrol. This sits in precedence between combat (a currently
// seen target) and patrol (the low-alert resting state).

// Bounded look-around after reaching the last-known spot (~2 s at 20 Hz). Named, no magic
// literal at the call site.
inline constexpr std::uint32_t kInvestigateDwellTicks = 40U;

// Same-origin nonvisual stimuli inside this horizontal radius refresh alert but do not restart
// the look-around dwell forever. Relocated sounds beyond it are treated as a new source.
inline constexpr float kNonvisualInvestigateRelocateEpsilonMeters = 0.75F;

// One investigate step. `active` false means "leave the decision to combat/patrol". When active,
// `dwelling` distinguishes looking-around-at-the-spot (hold) from walking-toward it (move).
struct NpcInvestigateStep {
  bool active = false;
  bool dwelling = false;
  Vec3 destination{};
};

// Record a fresh sighting: store the target position + tick as the memory and reset the dwell.
// Called every visually-confirmed tick so the memory tracks the latest sighting.
void npcRecordSighting(AiActorState& actor, Vec3 targetPosition, std::uint64_t tick);

// Record heard-only investigation memory. Returns true when the memory was refreshed:
//   - no existing memory,
//   - relocated horizontally beyond the epsilon,
//   - or a strict alert-band increase at the same origin.
// Same-origin sounds with no band increase preserve position/tick/dwell.
bool npcRecordNonvisualInvestigationMemory(
    AiActorState& actor, Vec3 stimulusPosition, std::uint64_t tick,
    bool alertBandIncreased,
    float relocateEpsilonMeters = kNonvisualInvestigateRelocateEpsilonMeters);

// Advance the investigate state and report the move/dwell. Pure except it mutates the actor's
// memory + dwell counter:
//   - no memory, or currently visually confirmed  -> inactive (combat/recording own the target)
//   - alertBand below Searching (< 3)              -> give up: clear memory, inactive
//   - band 3-4, memory, not confirmed:
//       far from last-known  -> approach (active, not dwelling, destination = last-known)
//       within arriveEpsilon -> dwell (++counter); on reaching dwellLimit give up, else hold.
NpcInvestigateStep npcStepInvestigate(AiActorState& actor, Vec3 actorPosition,
                                      std::uint8_t alertBand, bool visualConfirmed,
                                      float arriveEpsilonMeters,
                                      std::uint32_t dwellLimitTicks);

}  // namespace iggy3d
