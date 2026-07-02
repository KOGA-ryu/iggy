#include "runtime/ai/ReasoningRoute.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace iggy3d {
namespace {

float euclideanDistance(Vec3 a, Vec3 b) { return std::sqrt(lengthSquared(b - a)); }

// Nearest graph node reachable from `point` by a clear segment; ties -> lowest node id. a3s1 nodes
// are stable-sorted with id == index, so scanning `graph.nodes` in order is id-ascending and a
// STRICT `<` on distance keeps the lowest id on exact ties (e.g. the garden's co-located (1,1) pair
// -- both at the same position, distance identical -> the lower id wins). Returns false if no node
// is reachable.
bool nearestReachableNode(const ReasoningGraph& graph, std::span<const PhysicsAabbCollider> colliders,
                          Vec3 point, std::uint32_t& outId) {
  bool found = false;
  float bestDist = std::numeric_limits<float>::infinity();
  for (const ReasoningNode& node : graph.nodes) {
    if (reasoningSegmentBlocked(colliders, point, node.positionMeters)) {
      continue;
    }
    const float dist = euclideanDistance(point, node.positionMeters);
    if (!found || dist < bestDist) {
      found = true;
      bestDist = dist;
      outId = node.id;
    }
  }
  return found;
}

}  // namespace

float travelCost(const ReasoningEdge& edge, const TravelCostConfig& config) {
  const std::size_t kindIndex = static_cast<std::size_t>(edge.kind);
  const float multiplier =
      kindIndex < config.edgeKindMultipliers.size() ? config.edgeKindMultipliers[kindIndex] : 1.0F;
  constexpr float kNeutralSlope = 0.0F;  // v1 samples no slope (reserved term)
  return edge.lengthMeters * multiplier + config.slopePenaltyScale * kNeutralSlope;
}

PlannedRoute planRoute(const ReasoningGraph& graph, std::span<const PhysicsAabbCollider> colliders,
                       Vec3 fromMeters, Vec3 toMeters, const TravelCostConfig& config) {
  PlannedRoute route;
  const std::size_t nodeCount = graph.nodes.size();
  if (nodeCount == 0) {
    return route;  // empty graph -> no route
  }

  std::uint32_t entry = 0;
  std::uint32_t exit = 0;
  if (!nearestReachableNode(graph, colliders, fromMeters, entry) ||
      !nearestReachableNode(graph, colliders, toMeters, exit)) {
    return route;  // an endpoint has no reachable node -> empty (caller falls back)
  }
  if (entry == exit) {
    route.nodeIds.push_back(entry);
    return route;
  }

  // Undirected adjacency (edges cost the same both ways). a3s1 ids are the sorted index, so a node
  // id doubles as its dist/prev array index.
  std::vector<std::vector<std::pair<std::uint32_t, float>>> adjacency(nodeCount);
  for (const ReasoningEdge& edge : graph.edges) {
    if (edge.from < nodeCount && edge.to < nodeCount) {
      const float cost = travelCost(edge, config);
      adjacency[edge.from].push_back({edge.to, cost});
      adjacency[edge.to].push_back({edge.from, cost});
    }
  }

  // Dijkstra with O(V^2) selection (the graph is sparse + small; NOT a navmesh). Each round settles
  // the unsettled node with the least (cumulativeCost, id): scanning ids ascending with a STRICT `<`
  // makes the frontier order (cost, id) and settles ties lowest-id-first. Relaxation is also STRICT
  // so an equal-cost alternative never overwrites the lower-id predecessor -> deterministic path.
  constexpr float kInfinity = std::numeric_limits<float>::infinity();
  const std::uint32_t kNoNode = std::numeric_limits<std::uint32_t>::max();
  std::vector<float> dist(nodeCount, kInfinity);
  std::vector<std::uint32_t> prev(nodeCount, kNoNode);
  std::vector<bool> settled(nodeCount, false);
  dist[entry] = 0.0F;

  for (std::size_t round = 0; round < nodeCount; ++round) {
    std::uint32_t u = kNoNode;
    float best = kInfinity;
    for (std::uint32_t v = 0; v < nodeCount; ++v) {
      if (!settled[v] && dist[v] < best) {
        best = dist[v];
        u = v;
      }
    }
    if (u == kNoNode) {
      break;  // every remaining node is unreachable
    }
    settled[u] = true;
    if (u == exit) {
      break;  // exit is finalized
    }
    for (const std::pair<std::uint32_t, float>& link : adjacency[u]) {
      const std::uint32_t v = link.first;
      const float relaxed = dist[u] + link.second;
      if (!settled[v] && relaxed < dist[v]) {
        dist[v] = relaxed;
        prev[v] = u;
      }
    }
  }

  if (dist[exit] == kInfinity) {
    return route;  // exit disconnected from entry in the graph -> empty
  }

  // Reconstruct entry -> exit through settled predecessors.
  std::vector<std::uint32_t> reversed;
  for (std::uint32_t at = exit; at != kNoNode; at = prev[at]) {
    reversed.push_back(at);
    if (at == entry) {
      break;
    }
  }
  std::reverse(reversed.begin(), reversed.end());
  route.nodeIds = std::move(reversed);
  return route;
}

}  // namespace iggy3d
