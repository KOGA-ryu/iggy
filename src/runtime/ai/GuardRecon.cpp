#include "runtime/ai/GuardRecon.hpp"

namespace iggy3d {

GuardReconObservation projectGuardRecon(const AiActorState& guard,
                                        Vec3 guardPosition,
                                        const ReasoningGraph& graph) {
  GuardReconObservation obs;
  obs.guard = guard.actor;
  obs.position = guardPosition;
  obs.facing = guard.facingDirection;
  obs.alertLevel = guard.alertLevel;
  obs.behavior = aiBehaviorKindName(guard.behavior);

  obs.patrols = !guard.patrolWaypoints.empty();
  obs.patrolWaypointCount =
      static_cast<std::uint32_t>(guard.patrolWaypoints.size());
  obs.patrolTargetIndex = guard.patrolTargetIndex;
  obs.patrolMode = guard.patrolMode;
  if (obs.patrols && guard.patrolTargetIndex < guard.patrolWaypoints.size()) {
    obs.patrolTarget = guard.patrolWaypoints[guard.patrolTargetIndex];
  }

  obs.hasLastKnownTarget = guard.hasLastKnownTarget;
  obs.lastKnownTargetPosition = guard.lastKnownTargetPosition;

  // Watched node: a lookup by id -- never fabricate one. Unresolved (empty graph / no search
  // choice / stale id) leaves hasWatchedNode false.
  if (guard.hasSearchChoice) {
    for (const ReasoningNode& node : graph.nodes) {
      if (node.id == guard.searchChosenNodeId) {
        obs.hasWatchedNode = true;
        obs.watchedNodeKind = node.kind;
        obs.watchedNodePosition = node.positionMeters;
        break;
      }
    }
  }

  return obs;
}

}  // namespace iggy3d
