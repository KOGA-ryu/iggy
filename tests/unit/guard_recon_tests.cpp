#include "runtime/ai/GuardRecon.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool sameVec3(iggy3d::Vec3 a, iggy3d::Vec3 b) {
  return a.x == b.x && a.y == b.y && a.z == b.z;
}

// A guard mid-search: alerted, patrolling waypoint[1], remembers the intruder, heading to node 7.
iggy3d::AiActorState makeSeededGuard() {
  iggy3d::AiActorState guard;
  guard.actor = iggy3d::EntityId{42};
  guard.behavior = iggy3d::AiBehaviorKind::Searching;
  guard.facingDirection = iggy3d::Vec3{0.0F, 0.0F, -1.0F};
  guard.alertLevel = 0.75F;
  guard.patrolWaypoints = {
      iggy3d::Vec3{1.0F, 0.0F, 1.0F},
      iggy3d::Vec3{5.0F, 0.0F, 1.0F},
      iggy3d::Vec3{5.0F, 0.0F, 5.0F},
  };
  guard.patrolTargetIndex = 1;
  guard.hasLastKnownTarget = true;
  guard.lastKnownTargetPosition = iggy3d::Vec3{3.0F, 0.0F, 2.0F};
  guard.hasSearchChoice = true;
  guard.searchChosenNodeId = 7;
  return guard;
}

iggy3d::ReasoningGraph makeGraphWithNode(std::uint32_t id, iggy3d::Vec3 pos) {
  iggy3d::ReasoningGraph graph;
  iggy3d::ReasoningNode node;
  node.id = id;
  node.kind = iggy3d::ReasoningNodeKind::coverCluster;
  node.positionMeters = pos;
  graph.nodes.push_back(node);
  return graph;
}

bool projectsDurableGuardState() {
  const iggy3d::AiActorState guard = makeSeededGuard();
  const iggy3d::Vec3 pos{2.0F, 0.0F, 2.0F};
  const iggy3d::Vec3 nodePos{6.0F, 0.0F, 6.0F};
  const iggy3d::ReasoningGraph graph = makeGraphWithNode(7, nodePos);

  const iggy3d::GuardReconObservation obs =
      iggy3d::projectGuardRecon(guard, pos, graph);

  return expect(obs.guard == guard.actor, "guard id passthrough") &&
         expect(sameVec3(obs.position, pos), "position is the passed-in value") &&
         expect(sameVec3(obs.facing, guard.facingDirection), "facing passthrough") &&
         expect(obs.alertLevel == 0.75F, "alert level passthrough") &&
         expect(obs.behavior ==
                    iggy3d::aiBehaviorKindName(iggy3d::AiBehaviorKind::Searching),
                "behavior name from state") &&
         expect(obs.patrols, "patrols when waypoints present") &&
         expect(obs.patrolWaypointCount == 3U, "patrol waypoint count") &&
         expect(obs.patrolTargetIndex == 1U, "patrol target index") &&
         expect(sameVec3(obs.patrolTarget, guard.patrolWaypoints[1]),
                "patrol target is waypoint[index]") &&
         expect(obs.hasLastKnownTarget, "last-known carried") &&
         expect(sameVec3(obs.lastKnownTargetPosition, guard.lastKnownTargetPosition),
                "last-known position") &&
         expect(obs.hasWatchedNode, "watched node resolved") &&
         expect(obs.watchedNodeKind == iggy3d::ReasoningNodeKind::coverCluster,
                "watched node kind") &&
         expect(sameVec3(obs.watchedNodePosition, nodePos), "watched node position");
}

bool emptyGraphYieldsNoWatchedNode() {
  const iggy3d::AiActorState guard = makeSeededGuard();  // hasSearchChoice, id 7
  const iggy3d::GuardReconObservation obs =
      iggy3d::projectGuardRecon(guard, iggy3d::Vec3{}, iggy3d::ReasoningGraph{});
  return expect(!obs.hasWatchedNode,
                "empty graph -> no watched node (never fabricated)");
}

bool staleNodeIdYieldsNoWatchedNode() {
  iggy3d::AiActorState guard = makeSeededGuard();
  guard.searchChosenNodeId = 999;  // not in the graph
  const iggy3d::ReasoningGraph graph = makeGraphWithNode(7, iggy3d::Vec3{});
  const iggy3d::GuardReconObservation obs =
      iggy3d::projectGuardRecon(guard, iggy3d::Vec3{}, graph);
  return expect(!obs.hasWatchedNode, "unresolved node id -> no watched node");
}

bool noSearchChoiceYieldsNoWatchedNode() {
  iggy3d::AiActorState guard = makeSeededGuard();
  guard.hasSearchChoice = false;
  const iggy3d::ReasoningGraph graph = makeGraphWithNode(7, iggy3d::Vec3{});
  const iggy3d::GuardReconObservation obs =
      iggy3d::projectGuardRecon(guard, iggy3d::Vec3{}, graph);
  return expect(!obs.hasWatchedNode, "no search choice -> no watched node");
}

bool noPatrolIsClean() {
  const iggy3d::AiActorState guard;  // default: empty patrol
  const iggy3d::GuardReconObservation obs =
      iggy3d::projectGuardRecon(guard, iggy3d::Vec3{}, iggy3d::ReasoningGraph{});
  return expect(!obs.patrols, "no patrol when waypoints empty") &&
         expect(obs.patrolWaypointCount == 0U, "zero waypoint count");
}

}  // namespace

int main() {
  const bool ok = projectsDurableGuardState() && emptyGraphYieldsNoWatchedNode() &&
                  staleNodeIdYieldsNoWatchedNode() &&
                  noSearchChoiceYieldsNoWatchedNode() && noPatrolIsClean();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
