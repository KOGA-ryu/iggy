#pragma once

#include <vector>

#include "core/math/Vec3.hpp"
#include "runtime/ai/AiState.hpp"

namespace iggy3d {

// Low-alert patrol (slice 6 of docs/stealth-ai-plan.md). Pure free functions in the
// NpcAlertSystem style: an NPC with an authored waypoint route walks between the
// waypoints while it is NOT engaged. Patrol drives movement only; the actor's behavior
// stays the alert-derived rung, so this composes with the s5 escalation FSM (escalation
// always wins, patrol resumes only once alert has decayed to the Idle/Observant band).

// Horizontal distance within which a waypoint counts as reached. Named (no magic
// literal) and shared with the wiring as the Move stop distance so the NPC never
// overshoots the waypoint it is arriving at.
inline constexpr float kPatrolArriveEpsilonMeters = 0.25F;

// One step of the patrol cursor. When inactive the caller leaves the decision untouched.
struct NpcPatrolStep {
  bool active = false;
  Vec3 destination{};
};

// A route is valid when it is non-empty and every waypoint is finite. An empty route is
// simply "no patrol" (not an error); this predicate is the seed-time validation gate.
bool isValidPatrolRoute(const std::vector<Vec3>& waypoints);

// Advance the patrol cursor and report the point to move toward. Pure except it mutates
// the actor's cursor (patrolTargetIndex / patrolForward): if the actor is within
// arriveEpsilonMeters (horizontal) of the current waypoint it advances to the next per
// the mode (Loop wraps; PingPong bounces at the ends; a single-waypoint route holds).
// Returns {active=false} when the route is empty. Never reads NpcBehaviorSystem internals.
NpcPatrolStep npcStepPatrol(AiActorState& actor, Vec3 actorPosition,
                            float arriveEpsilonMeters);

}  // namespace iggy3d
