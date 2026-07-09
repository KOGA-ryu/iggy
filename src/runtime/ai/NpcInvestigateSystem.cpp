#include "runtime/ai/NpcInvestigateSystem.hpp"

#include <cmath>

namespace iggy3d {
namespace {

// Horizontal (x/z) distance. Investigate is a ground behavior; the remembered spot is reached
// when the guard stands over it regardless of height.
float horizontalDistanceMeters(Vec3 lhs, Vec3 rhs) {
  const float dx = rhs.x - lhs.x;
  const float dz = rhs.z - lhs.z;
  return std::sqrt(dx * dx + dz * dz);
}

// Below the Searching band the guard has calmed enough to stop investigating.
constexpr std::uint8_t kSearchingBand = 3U;

void clearMemory(AiActorState& actor) {
  actor.hasLastKnownTarget = false;
  actor.investigateDwellTicks = 0;
}

}  // namespace

void npcRecordSighting(AiActorState& actor, Vec3 targetPosition, std::uint64_t tick) {
  actor.lastKnownTargetPosition = targetPosition;
  actor.lastKnownTargetTick = tick;
  actor.hasLastKnownTarget = true;
  actor.investigateDwellTicks = 0;  // a fresh sighting restarts any look-around
}

bool npcRecordNonvisualInvestigationMemory(AiActorState& actor, Vec3 stimulusPosition,
                                           std::uint64_t tick, bool alertBandIncreased,
                                           float relocateEpsilonMeters) {
  if (!actor.hasLastKnownTarget ||
      horizontalDistanceMeters(actor.lastKnownTargetPosition, stimulusPosition) >
          relocateEpsilonMeters ||
      alertBandIncreased) {
    npcRecordSighting(actor, stimulusPosition, tick);
    return true;
  }
  return false;
}

NpcInvestigateStep npcStepInvestigate(AiActorState& actor, Vec3 actorPosition,
                                      std::uint8_t alertBand, bool visualConfirmed,
                                      float arriveEpsilonMeters,
                                      std::uint32_t dwellLimitTicks) {
  if (!actor.hasLastKnownTarget) {
    return {};
  }
  // A currently-seen target belongs to combat; recording (not investigating) tracks it.
  if (visualConfirmed) {
    return {};
  }
  // Alert has decayed out of the Searching band -> give up and hand back to patrol/idle.
  if (alertBand < kSearchingBand) {
    clearMemory(actor);
    return {};
  }

  NpcInvestigateStep step;
  step.active = true;
  step.destination = actor.lastKnownTargetPosition;

  const float distance = horizontalDistanceMeters(actorPosition, actor.lastKnownTargetPosition);
  if (distance > arriveEpsilonMeters) {
    // Still walking toward the remembered spot: reset the look-around.
    actor.investigateDwellTicks = 0;
    step.dwelling = false;
    return step;
  }

  // Standing on the spot: look around for a bounded time, then give up.
  ++actor.investigateDwellTicks;
  if (actor.investigateDwellTicks >= dwellLimitTicks) {
    clearMemory(actor);
    return {};
  }
  step.dwelling = true;
  return step;
}

}  // namespace iggy3d
