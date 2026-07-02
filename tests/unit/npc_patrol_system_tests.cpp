#include "runtime/ai/NpcPatrolSystem.hpp"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

#include "runtime/ai/AiState.hpp"

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::Vec3 v(float x, float z) { return iggy3d::Vec3{x, 0.0F, z}; }

bool sameXZ(iggy3d::Vec3 lhs, iggy3d::Vec3 rhs) {
  return std::fabs(lhs.x - rhs.x) <= 1.0e-4F && std::fabs(lhs.z - rhs.z) <= 1.0e-4F;
}

constexpr float kEps = iggy3d::kPatrolArriveEpsilonMeters;

bool routeValidation() {
  bool ok = expect(!iggy3d::isValidPatrolRoute({}), "empty route invalid");
  ok = ok && expect(iggy3d::isValidPatrolRoute({v(0.0F, 0.0F), v(1.0F, 0.0F)}),
                    "finite route valid");
  const float inf = std::numeric_limits<float>::infinity();
  ok = ok && expect(!iggy3d::isValidPatrolRoute({v(0.0F, 0.0F), {inf, 0.0F, 0.0F}}),
                    "non-finite waypoint invalid");
  const float nan = std::numeric_limits<float>::quiet_NaN();
  ok = ok && expect(!iggy3d::isValidPatrolRoute({{0.0F, nan, 0.0F}}),
                    "nan waypoint invalid");
  return ok;
}

bool emptyRouteInactive() {
  iggy3d::AiActorState actor;
  const iggy3d::NpcPatrolStep step = iggy3d::npcStepPatrol(actor, v(0.0F, 0.0F), kEps);
  return expect(!step.active, "empty route step inactive");
}

bool loopVisitsInOrderAndWraps() {
  iggy3d::AiActorState actor;
  actor.patrolMode = iggy3d::PatrolMode::Loop;
  actor.patrolWaypoints = {v(0.0F, 0.0F), v(2.0F, 0.0F), v(0.0F, 2.0F)};

  // Start on waypoint 0: each arrival advances to the next; loop wraps 2 -> 0.
  bool ok = true;
  const iggy3d::Vec3 expected[] = {v(2.0F, 0.0F), v(0.0F, 2.0F), v(0.0F, 0.0F),
                                   v(2.0F, 0.0F)};
  iggy3d::Vec3 at = v(0.0F, 0.0F);
  for (const iggy3d::Vec3& want : expected) {
    const iggy3d::NpcPatrolStep step = iggy3d::npcStepPatrol(actor, at, kEps);
    ok = ok && expect(step.active, "loop step active") &&
         expect(sameXZ(step.destination, want), "loop destination in order");
    at = step.destination;  // walk to the reported waypoint
  }
  return ok;
}

bool pingPongBouncesAtEnds() {
  iggy3d::AiActorState actor;
  actor.patrolMode = iggy3d::PatrolMode::PingPong;
  actor.patrolWaypoints = {v(0.0F, 0.0F), v(2.0F, 0.0F), v(4.0F, 0.0F)};

  bool ok = true;
  const iggy3d::Vec3 expected[] = {v(2.0F, 0.0F), v(4.0F, 0.0F), v(2.0F, 0.0F),
                                   v(0.0F, 0.0F), v(2.0F, 0.0F)};
  iggy3d::Vec3 at = v(0.0F, 0.0F);
  for (const iggy3d::Vec3& want : expected) {
    const iggy3d::NpcPatrolStep step = iggy3d::npcStepPatrol(actor, at, kEps);
    ok = ok && expect(sameXZ(step.destination, want), "ping-pong bounce sequence");
    at = step.destination;
  }
  return ok;
}

bool advancesOnlyWithinEpsilon() {
  iggy3d::AiActorState actor;
  actor.patrolMode = iggy3d::PatrolMode::Loop;
  actor.patrolWaypoints = {v(0.0F, 0.0F), v(5.0F, 0.0F)};
  actor.patrolTargetIndex = 1;  // walking toward (5,0)

  // Just outside epsilon of the target -> no advance, keep targeting it.
  iggy3d::NpcPatrolStep step =
      iggy3d::npcStepPatrol(actor, v(5.0F - (kEps + 0.05F), 0.0F), kEps);
  bool ok = expect(sameXZ(step.destination, v(5.0F, 0.0F)), "not arrived: hold target") &&
            expect(actor.patrolTargetIndex == 1U, "not arrived: index unchanged");

  // Within epsilon -> advance (Loop wraps 1 -> 0).
  step = iggy3d::npcStepPatrol(actor, v(5.0F - (kEps - 0.05F), 0.0F), kEps);
  ok = ok && expect(sameXZ(step.destination, v(0.0F, 0.0F)), "arrived within eps advances") &&
       expect(actor.patrolTargetIndex == 0U, "arrived: index advanced");
  return ok;
}

bool singleWaypointStationary() {
  iggy3d::AiActorState actor;
  actor.patrolWaypoints = {v(3.0F, 3.0F)};
  // From afar it targets the sole waypoint; on top of it it holds (never wraps away).
  iggy3d::NpcPatrolStep step = iggy3d::npcStepPatrol(actor, v(0.0F, 0.0F), kEps);
  bool ok = expect(step.active && sameXZ(step.destination, v(3.0F, 3.0F)),
                   "single waypoint targeted");
  step = iggy3d::npcStepPatrol(actor, v(3.0F, 3.0F), kEps);
  ok = ok && expect(sameXZ(step.destination, v(3.0F, 3.0F)), "single waypoint holds") &&
       expect(actor.patrolTargetIndex == 0U, "single waypoint index stays 0");
  return ok;
}

bool staleCursorClamped() {
  iggy3d::AiActorState actor;
  actor.patrolWaypoints = {v(0.0F, 0.0F), v(1.0F, 0.0F)};
  actor.patrolTargetIndex = 99;  // out of bounds -> must clamp, not crash
  const iggy3d::NpcPatrolStep step = iggy3d::npcStepPatrol(actor, v(9.0F, 9.0F), kEps);
  return expect(step.active && sameXZ(step.destination, v(0.0F, 0.0F)),
                "stale cursor clamped to first waypoint");
}

// s6c regression: the patrol Move parks the guard at kPatrolMoveStopMeters from the waypoint,
// which must be STRICTLY inside the arrival ring (kPatrolArriveEpsilonMeters) so arrival always
// fires there. With the old design (stop == epsilon) a cornered approach lands a float hair
// OUTSIDE the ring and the cursor stalls forever. Assert arrival fires at the rest distance and
// a few ULP beyond it (still < epsilon).
bool arrivesAtTheMoveRestDistance() {
  static_assert(iggy3d::kPatrolMoveStopMeters < iggy3d::kPatrolArriveEpsilonMeters,
                "rest distance must be inside the arrival ring");

  const auto advancesFrom = [](float distanceToWaypoint) {
    iggy3d::AiActorState actor;
    actor.patrolMode = iggy3d::PatrolMode::Loop;
    actor.patrolWaypoints = {v(5.0F, 0.0F), v(0.0F, 0.0F)};
    actor.patrolTargetIndex = 0;  // walking toward (5,0)
    // Place the actor `distanceToWaypoint` short of (5,0) along the approach axis.
    const iggy3d::NpcPatrolStep step =
        iggy3d::npcStepPatrol(actor, v(5.0F - distanceToWaypoint, 0.0F), kEps);
    // Advanced iff it now targets the NEXT waypoint (index 1 -> destination (0,0)).
    return sameXZ(step.destination, v(0.0F, 0.0F)) && actor.patrolTargetIndex == 1U;
  };

  const float ulp = std::nextafter(iggy3d::kPatrolMoveStopMeters, 1.0F) -
                    iggy3d::kPatrolMoveStopMeters;
  bool ok = expect(advancesFrom(iggy3d::kPatrolMoveStopMeters),
                   "arrival fires at the move rest distance") &&
            expect(advancesFrom(iggy3d::kPatrolMoveStopMeters + 4.0F * ulp),
                   "arrival fires a few ULP beyond the rest distance") &&
            // Sanity: still no early arrival just inside the epsilon boundary from outside.
            expect(!advancesFrom(iggy3d::kPatrolArriveEpsilonMeters + 0.05F),
                   "no arrival while still outside the ring");
  return ok;
}

}  // namespace

int main() {
  const bool ok = routeValidation() && emptyRouteInactive() &&
                  loopVisitsInOrderAndWraps() && pingPongBouncesAtEnds() &&
                  advancesOnlyWithinEpsilon() && singleWaypointStationary() &&
                  staleCursorClamped() && arrivesAtTheMoveRestDistance();
  return ok ? 0 : 1;
}
