#include "runtime/ai/InfluenceMap.hpp"

#include <cmath>
#include <limits>
#include <queue>
#include <utility>

#include "runtime/ai/ReasoningRoute.hpp"

namespace iggy3d {
namespace {

float distanceMeters(Vec3 a, Vec3 b) { return std::sqrt(lengthSquared(b - a)); }

std::size_t channelIndex(InfluenceChannel channel) { return static_cast<std::size_t>(channel); }

// exp(-dGraph / decayScale): 1.0 at the seed (dGraph 0), decaying with graph distance; 0.0 when the
// node is unreachable from every seed (dGraph == +inf -> honest: no reachable seed, no field).
float graphDecay(float graphDistance, float decayScale) {
  if (!std::isfinite(graphDistance)) {
    return 0.0F;
  }
  if (decayScale <= 0.0F) {
    return graphDistance <= 0.0F ? 1.0F : 0.0F;
  }
  return std::exp(-graphDistance / decayScale);
}

// Least graph distance from every node to the NEAREST node whose kind == seedKind. Multi-source
// Dijkstra over UNDIRECTED edges (weight = travelCost). Adjacency is built in INDEX space via an
// explicit idToIndex map (no silent id==index assumption). Frontier orders by (dist, nodeId) so
// equal-dist settle lowest-id-first (the planRoute determinism discipline). Unreached -> +inf.
std::vector<float> graphDistanceToKind(const ReasoningGraph& graph, ReasoningNodeKind seedKind) {
  const std::size_t nodeCount = graph.nodes.size();
  std::vector<float> dist(nodeCount, std::numeric_limits<float>::infinity());
  if (nodeCount == 0) {
    return dist;
  }

  std::uint32_t maxId = 0;
  for (const ReasoningNode& node : graph.nodes) {
    maxId = std::max(maxId, node.id);
  }
  constexpr std::size_t kNoIndex = static_cast<std::size_t>(-1);
  std::vector<std::size_t> idToIndex(static_cast<std::size_t>(maxId) + 1U, kNoIndex);
  for (std::size_t i = 0; i < nodeCount; ++i) {
    idToIndex[graph.nodes[i].id] = i;
  }

  std::vector<std::vector<std::pair<std::size_t, float>>> adjacency(nodeCount);
  for (const ReasoningEdge& edge : graph.edges) {
    if (edge.from > maxId || edge.to > maxId) {
      continue;
    }
    const std::size_t from = idToIndex[edge.from];
    const std::size_t to = idToIndex[edge.to];
    if (from == kNoIndex || to == kNoIndex) {
      continue;
    }
    const float weight = travelCost(edge, {});
    adjacency[from].push_back({to, weight});
    adjacency[to].push_back({from, weight});
  }

  struct Frontier {
    float dist;
    std::uint32_t nodeId;
    std::size_t index;
  };
  const auto later = [](const Frontier& lhs, const Frontier& rhs) {
    if (lhs.dist != rhs.dist) {
      return lhs.dist > rhs.dist;  // min pops first
    }
    return lhs.nodeId > rhs.nodeId;  // ties settle lowest-id-first
  };
  std::priority_queue<Frontier, std::vector<Frontier>, decltype(later)> frontier(later);
  for (std::size_t i = 0; i < nodeCount; ++i) {
    if (graph.nodes[i].kind == seedKind) {
      dist[i] = 0.0F;
      frontier.push({0.0F, graph.nodes[i].id, i});
    }
  }

  while (!frontier.empty()) {
    const Frontier top = frontier.top();
    frontier.pop();
    if (top.dist > dist[top.index]) {
      continue;  // stale entry
    }
    for (const std::pair<std::size_t, float>& link : adjacency[top.index]) {
      const float relaxed = dist[top.index] + link.second;
      if (relaxed < dist[link.first]) {
        dist[link.first] = relaxed;
        frontier.push({relaxed, graph.nodes[link.first].id, link.first});
      }
    }
  }
  return dist;
}

}  // namespace

std::string_view influenceChannelName(InfluenceChannel channel) {
  switch (channel) {
    case InfluenceChannel::guardInfluence: return "guardInfluence";
    case InfluenceChannel::visibilityCoverage: return "visibilityCoverage";
    case InfluenceChannel::escapeRoutePressure: return "escapeRoutePressure";
    case InfluenceChannel::objectiveControl: return "objectiveControl";
    case InfluenceChannel::danger: return "danger";
    case InfluenceChannel::soundPressure: return "soundPressure";
  }
  return "unknown";
}

InfluenceMap buildInfluenceMap(const ReasoningGraph& graph,
                               std::span<const PhysicsAabbCollider> colliders,
                               std::span<const InfluenceGuardSample> guards,
                               const InfluenceMapConfig& config) {
  InfluenceMap map;
  map.nodeCount = graph.nodes.size();
  map.values.assign(map.nodeCount, std::array<float, kInfluenceChannelCount>{});
  if (map.nodeCount == 0) {
    return map;
  }

  // Static channels: one multi-source pass each (edge weight = travelCost).
  const std::vector<float> distToExit = graphDistanceToKind(graph, ReasoningNodeKind::exit);
  const std::vector<float> distToObjective = graphDistanceToKind(graph, ReasoningNodeKind::objective);

  for (std::size_t i = 0; i < map.nodeCount; ++i) {
    const Vec3 nodePos = graph.nodes[i].positionMeters;
    std::array<float, kInfluenceChannelCount>& row = map.values[i];

    // guardInfluence (DYNAMIC): summed linear presence, exactly 0 at/beyond the radius.
    float guardInfluence = 0.0F;
    for (const InfluenceGuardSample& guard : guards) {
      const float d = distanceMeters(guard.positionMeters, nodePos);
      if (config.guardInfluenceRadiusMeters > 0.0F) {
        const float falloff = 1.0F - d / config.guardInfluenceRadiusMeters;
        if (falloff > 0.0F) {
          guardInfluence += falloff;
        }
      }
    }
    row[channelIndex(InfluenceChannel::guardInfluence)] = guardInfluence;

    // visibilityCoverage (DYNAMIC, POTENTIAL/omnidirectional): radius + clear LOS from ANY guard.
    float visibility = 0.0F;
    for (const InfluenceGuardSample& guard : guards) {
      if (distanceMeters(guard.positionMeters, nodePos) <= config.visibilityRadiusMeters &&
          !reasoningSegmentBlocked(colliders, guard.positionMeters, nodePos)) {
        visibility = 1.0F;
        break;
      }
    }
    row[channelIndex(InfluenceChannel::visibilityCoverage)] = visibility;

    // Static channels: graph-distance decay to the nearest exit / objective.
    row[channelIndex(InfluenceChannel::escapeRoutePressure)] =
        graphDecay(distToExit[i], config.escapePressureDecayMeters);
    row[channelIndex(InfluenceChannel::objectiveControl)] =
        graphDecay(distToObjective[i], config.objectiveControlDecayMeters);

    // danger + soundPressure stay 0.0 (reserved, not computed v1).
  }

  return map;
}

float influenceValue(const InfluenceMap& map, std::size_t nodeIndex, InfluenceChannel channel) {
  if (nodeIndex >= map.values.size()) {
    return 0.0F;
  }
  const std::size_t channelIdx = channelIndex(channel);
  if (channelIdx >= kInfluenceChannelCount) {
    return 0.0F;
  }
  return map.values[nodeIndex][channelIdx];
}

}  // namespace iggy3d
