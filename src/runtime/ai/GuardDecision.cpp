#include "runtime/ai/GuardDecision.hpp"

#include <cmath>

#include "runtime/ai/ReasoningRoute.hpp"

namespace iggy3d {
namespace {

float euclideanDistance(Vec3 a, Vec3 b) { return std::sqrt(lengthSquared(b - a)); }

// Linear proximity: 1 at the memory spot, falling to 0 at the falloff radius (clamped). No magic --
// the radius is named config.
float proximity01(float distanceMeters, float falloffRadiusMeters) {
  if (falloffRadiusMeters <= 0.0F) {
    return 0.0F;
  }
  const float p = 1.0F - distanceMeters / falloffRadiusMeters;
  return p > 0.0F ? p : 0.0F;
}

// Exponential staleness: a memory decays by half every halfLife ticks. Named config; fresh (dt=0)
// gives 1.0.
float stalenessDecay(std::uint64_t ticksSinceRecorded, float halfLifeTicks) {
  if (halfLifeTicks <= 0.0F) {
    return 0.0F;
  }
  return std::pow(0.5F, static_cast<float>(ticksSinceRecorded) / halfLifeTicks);
}

// travelTerm for a candidate node. Direct clear segment => straight-line distance; else the A4 route
// cost. `reachable` is set false when the direct is blocked AND no route exists (unreachable node).
float travelTerm(const ReasoningGraph& graph, std::span<const PhysicsAabbCollider> colliders,
                 Vec3 guardPosition, Vec3 nodePosition, bool& reachable) {
  reachable = true;
  if (!reasoningSegmentBlocked(colliders, guardPosition, nodePosition)) {
    return euclideanDistance(guardPosition, nodePosition);
  }
  const PlannedRoute route = planRoute(graph, colliders, guardPosition, nodePosition, {});
  if (route.nodeIds.empty()) {
    reachable = false;  // blocked direct + no route -> unreachable
    return 0.0F;
  }
  return route.totalCostMeters;
}

}  // namespace

GuardDecision chooseSearchNode(const ReasoningGraph& graph,
                               std::span<const PhysicsAabbCollider> colliders, Vec3 guardPosition,
                               const GuardMemorySample& memory, std::uint64_t tick, EntityId actor,
                               const NpcPersonalityWeights& weights, const GuardDecisionConfig& config,
                               std::optional<std::uint32_t> excludedNodeId) {
  GuardDecision decision;
  decision.receipt.actor = actor;
  decision.receipt.tick = tick;
  if (excludedNodeId.has_value()) {
    decision.receipt.hasExcluded = true;
    decision.receipt.excludedNodeId = *excludedNodeId;
  }

  const float staleness =
      memory.hasMemorySample
          ? stalenessDecay(tick >= memory.lastKnownTick ? tick - memory.lastKnownTick : 0,
                           config.suspicionStalenessHalfLifeTicks)
          : 0.0F;

  bool hasBest = false;
  float bestScore = 0.0F;
  std::uint32_t bestId = 0;
  float bestSuspicion = 0.0F;
  float bestStrategic = 0.0F;
  float bestTravel = 0.0F;

  // Nodes are id-sorted (id == index); iterating in order + a STRICT `>` update makes equal scores
  // resolve to the lowest id (co-located nodes tie legitimately).
  for (const ReasoningNode& node : graph.nodes) {
    if (excludedNodeId.has_value() && node.id == *excludedNodeId) {
      continue;
    }
    bool reachable = true;
    const float travel = travelTerm(graph, colliders, guardPosition, node.positionMeters, reachable);
    if (!reachable) {
      continue;  // unreachable candidate excluded from scoring
    }
    const float suspicion =
        memory.hasMemorySample
            ? proximity01(euclideanDistance(node.positionMeters, memory.lastKnownPosition),
                          config.suspicionFalloffRadiusMeters) *
                  staleness
            : 0.0F;
    const std::size_t kindIndex = static_cast<std::size_t>(node.kind);
    const float strategic = kindIndex < config.strategicValueByKind.size()
                                ? config.strategicValueByKind[kindIndex]
                                : 0.0F;
    const float score = suspicion * weights.suspicionWeight + strategic * weights.strategicWeight -
                        travel * weights.travelWeight + 0.0F * weights.allyWeight;
    if (!hasBest || score > bestScore) {
      hasBest = true;
      bestScore = score;
      bestId = node.id;
      bestSuspicion = suspicion;
      bestStrategic = strategic;
      bestTravel = travel;
    }
  }

  if (!hasBest) {
    return decision;  // nodeId = nullopt, receipt.hasChoice = false
  }

  decision.nodeId = bestId;
  decision.receipt.hasChoice = true;
  decision.receipt.chosenNodeId = bestId;
  decision.receipt.totalScore = bestScore;
  // Signed factors, each already multiplied by its weight (the ACTUAL contribution). ally is a hard
  // 0 in v1 -- emitted so the receipt SHAPE is final for A9.
  decision.receipt.factors = {
      GuardDecisionFactor{"suspicion", bestSuspicion * weights.suspicionWeight},
      GuardDecisionFactor{"strategic", bestStrategic * weights.strategicWeight},
      GuardDecisionFactor{"travel", -(bestTravel * weights.travelWeight)},
      GuardDecisionFactor{"ally", 0.0F * weights.allyWeight},
  };
  return decision;
}

}  // namespace iggy3d
