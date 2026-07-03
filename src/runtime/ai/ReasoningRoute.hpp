#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "core/math/Vec3.hpp"
#include "runtime/ai/ReasoningGraph.hpp"
#include "runtime/movement/MovementCapability.hpp"
#include "runtime/physics/PhysicsAabbCollider.hpp"

namespace iggy3d {

// L5 travel cost + routing over the L4 reasoning graph (A4). Pure, deterministic, ACTOR-FREE:
// map v1.3 §5's travelCost(actor, node) is SUPERSEDED by this actor-free edge-cost form -- all
// actor-dependent terms (slope, personality) are reserved-neutral and land with their tuning
// slices. ZERO behavior change: nothing consumes these yet (a4s2 wires planRoute into guard
// movement). A route is a sequence of node ids; an EMPTY route means "no route" so callers fall
// back to today's behavior -- routing NEVER makes a guard worse than the status quo.

// Unit (neutral) per-edge-kind multipliers -- the default cost table. Sized/ordered to
// ReasoningEdgeKind (walkable..guarded).
inline constexpr std::array<float, kReasoningEdgeKindCount> kUnitEdgeKindMultipliers{
    1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F};
static_assert(kReasoningEdgeKindCount == 7,
              "kUnitEdgeKindMultipliers must list one multiplier per ReasoningEdgeKind");

// Named cost tuning (no magic numbers). v1 graphs emit only `walkable`, but the full per-kind table
// exists so noisy/dangerous/guarded cost tuning lands WITHOUT re-cutting L5.
struct TravelCostConfig {
  std::array<float, kReasoningEdgeKindCount> edgeKindMultipliers = kUnitEdgeKindMultipliers;
  // RESERVED-NEUTRAL slope term (map's sampleSlope/SlopeBand). v1 samples no slope and queries no
  // collision here; a later tuning slice wires it. Default 0 => no contribution.
  float slopePenaltyScale = 0.0F;
};

// MA4 (M3): the UNUSABLE sentinel -- a multiplier marking an edge kind the router must SKIP entirely
// (NOT infinity arithmetic; map v1.3 correction). Negative so it is unambiguous against every real
// positive multiplier. InfluenceMap's SEPARATE +inf-for-unreached convention is untouched -- this
// scopes only to the router's edge-kind gate.
inline constexpr float kUnusableEdgeKindMultiplier = -1.0F;

// True unless `kind`'s multiplier is the unusable sentinel. planRoute + InfluenceMap's Dijkstra
// `continue` (skip the edge) when this is false -- BEFORE travelCost, so the sentinel never enters a
// cost sum (planRoute-returned costs stay finite + sentinel-free).
inline bool isTraversableEdgeKind(const TravelCostConfig& config, ReasoningEdgeKind kind) {
  const std::size_t index = static_cast<std::size_t>(kind);
  return index >= config.edgeKindMultipliers.size() ||
         config.edgeKindMultipliers[index] != kUnusableEdgeKindMultiplier;
}

// MA4: the class -> cost-row mapping (ai owns it; ai includes movement for the enum). `grounded`
// marks `climb` UNUSABLE (walks around); `climber` leaves climb at 1.0 (routes over). leaper/flier
// are RESERVED and return grounded's config (no v1 behavior). Pure, deterministic.
TravelCostConfig travelCostConfigForCapability(MovementCapabilityClass capability);

// Actor-free edge cost: euclidean length x the edge-kind multiplier (+ the neutral slope term = 0).
float travelCost(const ReasoningEdge& edge, const TravelCostConfig& config = {});

// A planned route as an ordered node-id sequence. EMPTY nodeIds => no route (caller falls back).
struct PlannedRoute {
  std::vector<std::uint32_t> nodeIds;
  // Total traversal cost of the route: the guard->entry straight leg PLUS the sum of edge
  // travelCosts entry->exit (A5 L6 travel term reads this). EMPTY route => 0. When `to` is itself a
  // graph node the exit node IS `to`, so there is no exit-leg double count.
  float totalCostMeters = 0.0F;
};

// Least-cost route between two world positions over `graph`. Takes ALREADY-BAKED colliders (a4s2
// passes the seam's visionColliders) -- NEVER bakes. ENTRY/EXIT = nearest node reachable by a clear
// segment (reasoningSegmentBlocked; ties -> lowest node id). PATH = least-total-travelCost over the
// UNDIRECTED edges (Dijkstra over the sparse graph -- NOT a navmesh; frontier ordered by
// (cumulativeCost, nodeId) so equal-cost candidates settle lowest-id-first). Unreachable endpoint
// or a disconnected exit -> EMPTY. entry == exit -> single-node route [entry]. Pure/deterministic:
// identical inputs -> bitwise-identical nodeIds.
PlannedRoute planRoute(const ReasoningGraph& graph, std::span<const PhysicsAabbCollider> colliders,
                       Vec3 fromMeters, Vec3 toMeters, const TravelCostConfig& config = {});

}  // namespace iggy3d
