#include "runtime/ai/NpcInvestigateSystem.hpp"

#include <cstdint>
#include <iostream>
#include <string_view>

#include "runtime/ai/AiState.hpp"
#include "runtime/ai/NpcPatrolSystem.hpp"  // kPatrolArriveEpsilonMeters (the wiring's arrival ring)

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::Vec3 v(float x, float z) { return iggy3d::Vec3{x, 0.0F, z}; }

constexpr float kEps = iggy3d::kPatrolArriveEpsilonMeters;  // arrival ring (0.25 m)
constexpr std::uint8_t kSearching = 3U;  // band at which investigate is active
constexpr std::uint8_t kIdleBand = 0U;

bool recordSightingSetsMemory() {
  iggy3d::AiActorState actor;
  actor.investigateDwellTicks = 7;  // must reset on a fresh sighting
  iggy3d::npcRecordSighting(actor, v(4.0F, 2.0F), 99U);
  return expect(actor.hasLastKnownTarget, "sighting sets memory flag") &&
         expect(actor.lastKnownTargetPosition.x == 4.0F &&
                    actor.lastKnownTargetPosition.z == 2.0F,
                "sighting stores position") &&
         expect(actor.lastKnownTargetTick == 99U, "sighting stores tick") &&
         expect(actor.investigateDwellTicks == 0U, "sighting resets dwell");
}

bool noMemoryIsInactive() {
  iggy3d::AiActorState actor;  // hasLastKnownTarget defaults false
  const iggy3d::NpcInvestigateStep step = iggy3d::npcStepInvestigate(
      actor, v(0.0F, 0.0F), kSearching, false, kEps, iggy3d::kInvestigateDwellTicks);
  return expect(!step.active, "no memory -> inactive");
}

bool visualConfirmedIsInactive() {
  iggy3d::AiActorState actor;
  iggy3d::npcRecordSighting(actor, v(3.0F, 0.0F), 1U);
  // Currently seeing the target: combat owns it, investigate steps aside (but memory stays).
  const iggy3d::NpcInvestigateStep step = iggy3d::npcStepInvestigate(
      actor, v(0.0F, 0.0F), kSearching, true, kEps, iggy3d::kInvestigateDwellTicks);
  return expect(!step.active, "visually confirmed -> inactive") &&
         expect(actor.hasLastKnownTarget, "confirmed does not clear memory");
}

bool belowSearchingGivesUp() {
  iggy3d::AiActorState actor;
  iggy3d::npcRecordSighting(actor, v(3.0F, 0.0F), 1U);
  const iggy3d::NpcInvestigateStep step = iggy3d::npcStepInvestigate(
      actor, v(0.0F, 0.0F), kIdleBand, false, kEps, iggy3d::kInvestigateDwellTicks);
  return expect(!step.active, "below searching -> inactive") &&
         expect(!actor.hasLastKnownTarget, "below searching clears memory (give up)");
}

// The full lifecycle: record -> lose sight (approach) -> arrive (dwell, counter climbs) ->
// dwell elapses (give up, memory cleared).
bool fullTransitionApproachDwellGiveUp() {
  iggy3d::AiActorState actor;
  iggy3d::npcRecordSighting(actor, v(5.0F, 0.0F), 10U);

  // Lost sight, still Searching, far from the spot -> approach (move toward last-known).
  iggy3d::NpcInvestigateStep step = iggy3d::npcStepInvestigate(
      actor, v(0.0F, 0.0F), kSearching, false, kEps, iggy3d::kInvestigateDwellTicks);
  bool ok = expect(step.active && !step.dwelling, "far -> approach") &&
            expect(step.destination.x == 5.0F && step.destination.z == 0.0F,
                   "approach targets last-known") &&
            expect(actor.investigateDwellTicks == 0U, "approach keeps dwell at 0");

  // Standing on the spot -> dwell; the counter climbs each tick short of the limit.
  for (std::uint32_t i = 1; i < iggy3d::kInvestigateDwellTicks; ++i) {
    step = iggy3d::npcStepInvestigate(actor, v(5.0F, 0.0F), kSearching, false, kEps,
                                      iggy3d::kInvestigateDwellTicks);
    ok = ok && expect(step.active && step.dwelling, "on spot -> dwelling") &&
         expect(actor.investigateDwellTicks == i, "dwell counter climbs");
  }

  // The tick that reaches the dwell limit -> give up, memory cleared, inactive.
  step = iggy3d::npcStepInvestigate(actor, v(5.0F, 0.0F), kSearching, false, kEps,
                                    iggy3d::kInvestigateDwellTicks);
  ok = ok && expect(!step.active, "dwell elapsed -> inactive") &&
       expect(!actor.hasLastKnownTarget, "dwell elapsed clears memory");
  return ok;
}

// Re-approaching (walking away from the spot again) resets the look-around dwell.
bool leavingSpotResetsDwell() {
  iggy3d::AiActorState actor;
  iggy3d::npcRecordSighting(actor, v(5.0F, 0.0F), 1U);
  // A couple of dwell ticks on the spot.
  (void)iggy3d::npcStepInvestigate(actor, v(5.0F, 0.0F), kSearching, false, kEps,
                                   iggy3d::kInvestigateDwellTicks);
  bool ok = expect(actor.investigateDwellTicks == 1U, "one dwell tick recorded");
  // Now away from the spot again -> approach resets the counter.
  const iggy3d::NpcInvestigateStep step = iggy3d::npcStepInvestigate(
      actor, v(0.0F, 0.0F), kSearching, false, kEps, iggy3d::kInvestigateDwellTicks);
  ok = ok && expect(step.active && !step.dwelling, "away again -> approach") &&
       expect(actor.investigateDwellTicks == 0U, "leaving the spot resets dwell");
  return ok;
}

}  // namespace

int main() {
  const bool ok = recordSightingSetsMemory() && noMemoryIsInactive() &&
                  visualConfirmedIsInactive() && belowSearchingGivesUp() &&
                  fullTransitionApproachDwellGiveUp() && leavingSpotResetsDwell();
  return ok ? 0 : 1;
}
