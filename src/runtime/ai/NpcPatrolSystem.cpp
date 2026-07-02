#include "runtime/ai/NpcPatrolSystem.hpp"

#include <cmath>

namespace iggy3d {
namespace {

// Horizontal (x/z) distance. Patrol is a ground behavior; height is ignored so a
// waypoint authored at any y still counts as reached when the NPC walks over it.
float horizontalDistanceMeters(Vec3 lhs, Vec3 rhs) {
  const float dx = rhs.x - lhs.x;
  const float dz = rhs.z - lhs.z;
  return std::sqrt(dx * dx + dz * dz);
}

// Move the cursor to the next waypoint per the mode. Loop wraps; PingPong reverses at
// each end. A single-waypoint route is a no-op (the NPC just holds on it).
void advancePatrolCursor(AiActorState& actor) {
  const std::size_t count = actor.patrolWaypoints.size();
  if (count <= 1U) {
    actor.patrolTargetIndex = 0;
    return;
  }
  if (actor.patrolMode == PatrolMode::Loop) {
    actor.patrolTargetIndex =
        static_cast<std::uint32_t>((actor.patrolTargetIndex + 1U) % count);
    return;
  }
  // PingPong: step in the current direction, flipping at either end.
  if (actor.patrolForward) {
    if (actor.patrolTargetIndex + 1U < count) {
      ++actor.patrolTargetIndex;
    } else {
      actor.patrolForward = false;
      actor.patrolTargetIndex = static_cast<std::uint32_t>(count - 2U);
    }
  } else {
    if (actor.patrolTargetIndex > 0U) {
      --actor.patrolTargetIndex;
    } else {
      actor.patrolForward = true;
      actor.patrolTargetIndex = 1U;
    }
  }
}

}  // namespace

bool isValidPatrolRoute(const std::vector<Vec3>& waypoints) {
  if (waypoints.empty()) {
    return false;
  }
  for (const Vec3& waypoint : waypoints) {
    if (!std::isfinite(waypoint.x) || !std::isfinite(waypoint.y) ||
        !std::isfinite(waypoint.z)) {
      return false;
    }
  }
  return true;
}

NpcPatrolStep npcStepPatrol(AiActorState& actor, Vec3 actorPosition,
                            float arriveEpsilonMeters) {
  const std::size_t count = actor.patrolWaypoints.size();
  if (count == 0U) {
    return {};
  }
  // Defensive clamp: a stale cursor (e.g. a route shrunk out from under us) must never
  // index out of bounds.
  if (actor.patrolTargetIndex >= count) {
    actor.patrolTargetIndex = 0;
  }

  const float distance =
      horizontalDistanceMeters(actorPosition, actor.patrolWaypoints[actor.patrolTargetIndex]);
  if (distance <= arriveEpsilonMeters) {
    advancePatrolCursor(actor);
  }

  NpcPatrolStep step;
  step.active = true;
  step.destination = actor.patrolWaypoints[actor.patrolTargetIndex];
  return step;
}

}  // namespace iggy3d
