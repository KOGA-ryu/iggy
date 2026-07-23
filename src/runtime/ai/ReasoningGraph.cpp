#include "runtime/ai/ReasoningGraph.hpp"

#include <algorithm>
#include <cmath>
#include <tuple>

#include "runtime/ai/SegmentOcclusion.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/physics/PhysicsSpatialSurfaceColliderBake.hpp"

namespace iggy3d {
namespace {

// Anchor-kind -> node-kind mapping (the affordance vocabulary v0.1 wire strings; docs/
// affordance_vocabulary_v0_1.md is the contract). Returns false for kinds with no v0.1 node --
// door/secret_door (a3s1b `doorway`), marker, and the deck-only `trap`/`reset_zone` -- and for any
// unknown string: NO node, NO error (ignore-and-continue is the contract's law). Stable node
// ordering (sort by kind,x,z,y) is unchanged. DORMANT until an authored map emits these strings.
bool nodeKindForAnchor(const std::string& anchorKind, ReasoningNodeKind& out) {
  if (anchorKind == "exit") {
    out = ReasoningNodeKind::exit;
    return true;
  }
  if (anchorKind == "treasure" || anchorKind == "key" || anchorKind == "pickup") {
    out = ReasoningNodeKind::objective;
    return true;
  }
  if (anchorKind == "spawn" || anchorKind == "npc" || anchorKind == "monster") {
    out = ReasoningNodeKind::reference;
    return true;
  }
  if (anchorKind == "chokepoint") {
    out = ReasoningNodeKind::chokepoint;
    return true;
  }
  if (anchorKind == "high_ground") {
    out = ReasoningNodeKind::highGround;
    return true;
  }
  if (anchorKind == "hiding_spot") {
    out = ReasoningNodeKind::hidingSpot;
    return true;
  }
  if (anchorKind == "cover") {
    out = ReasoningNodeKind::coverCluster;
    return true;
  }
  if (anchorKind == "patrol_post") {
    out = ReasoningNodeKind::patrolPost;
    return true;
  }
  if (anchorKind == "stair") {
    out = ReasoningNodeKind::stair;
    return true;
  }
  if (anchorKind == "ramp") {
    out = ReasoningNodeKind::ramp;
    return true;
  }
  return false;
}

}  // namespace

bool reasoningSegmentBlocked(
    std::span<const PhysicsAabbCollider> colliders,
    Vec3 from,
    Vec3 to,
    float eyeHeightMeters,
    float marginMeters) {
  Vec3 origin = from;
  origin.y += eyeHeightMeters;
  Vec3 dest = to;
  dest.y += eyeHeightMeters;
  const SegmentOcclusionVerdict verdict =
      segmentOcclusion(colliders, origin, dest, marginMeters);
  if (verdict == SegmentOcclusionVerdict::Clear) {
    return false;
  }
  return true;
}

std::string_view reasoningNodeKindName(ReasoningNodeKind kind) {
  switch (kind) {
    case ReasoningNodeKind::doorway: return "doorway";
    case ReasoningNodeKind::stair: return "stair";
    case ReasoningNodeKind::chokepoint: return "chokepoint";
    case ReasoningNodeKind::hidingSpot: return "hidingSpot";
    case ReasoningNodeKind::coverCluster: return "coverCluster";
    case ReasoningNodeKind::objective: return "objective";
    case ReasoningNodeKind::window: return "window";
    case ReasoningNodeKind::ladder: return "ladder";
    case ReasoningNodeKind::exit: return "exit";
    case ReasoningNodeKind::patrolPost: return "patrolPost";
    case ReasoningNodeKind::highGround: return "highGround";
    case ReasoningNodeKind::soundSource: return "soundSource";
    case ReasoningNodeKind::lastKnownPosition: return "lastKnownPosition";
    case ReasoningNodeKind::reference: return "reference";
    case ReasoningNodeKind::ramp: return "ramp";
  }
  return "unknown";
}

std::string_view reasoningEdgeKindName(ReasoningEdgeKind kind) {
  switch (kind) {
    case ReasoningEdgeKind::walkable: return "walkable";
    case ReasoningEdgeKind::hidden: return "hidden";
    case ReasoningEdgeKind::climb: return "climb";
    case ReasoningEdgeKind::locked: return "locked";
    case ReasoningEdgeKind::noisy: return "noisy";
    case ReasoningEdgeKind::dangerous: return "dangerous";
    case ReasoningEdgeKind::guarded: return "guarded";
  }
  return "unknown";
}

ReasoningGraphSummary summarizeReasoningGraph(const ReasoningGraph& graph) {
  ReasoningGraphSummary summary;
  summary.nodeCount = graph.nodes.size();
  summary.edgeCount = graph.edges.size();
  for (const ReasoningNode& node : graph.nodes) {
    const std::size_t index = static_cast<std::size_t>(node.kind);
    if (index < summary.perKindCounts.size()) {
      ++summary.perKindCounts[index];
    }
  }
  return summary;
}

ReasoningGraph buildReasoningGraph(const RoomAsset& room, std::span<const Vec3> patrolWaypoints,
                                   const ReasoningGraphConfig& config) {
  ReasoningGraph graph;

  // 1. Collect nodes: derivable anchors, then one patrolPost per caller-supplied waypoint.
  for (const RoomAnchorAsset& anchor : room.anchors) {
    ReasoningNodeKind kind = ReasoningNodeKind::reference;
    if (nodeKindForAnchor(anchor.kind, kind)) {
      graph.nodes.push_back(ReasoningNode{0U, kind, anchor.positionMeters, anchor.kind});
    }
  }
  for (const Vec3& waypoint : patrolWaypoints) {
    graph.nodes.push_back(ReasoningNode{0U, ReasoningNodeKind::patrolPost, waypoint, "waypoint"});
  }

  // 2. Stable order (determinism law): sort by (kind, x, z, y) then id = sorted index. This makes
  // the ids -- and therefore every edge -- bitwise-reproducible for identical inputs.
  const auto sortKey = [](const ReasoningNode& node) {
    return std::make_tuple(static_cast<std::uint8_t>(node.kind), node.positionMeters.x,
                           node.positionMeters.z, node.positionMeters.y);
  };
  std::sort(graph.nodes.begin(), graph.nodes.end(),
            [&sortKey](const ReasoningNode& a, const ReasoningNode& b) {
              return sortKey(a) < sortKey(b);
            });
  for (std::size_t i = 0; i < graph.nodes.size(); ++i) {
    graph.nodes[i].id = static_cast<std::uint32_t>(i);
  }

  // 3. Walkable edges. Bake the actor-blocking colliders ONCE from the room geometry -- the same
  // colliders vision and hearing use. All-pairs over the sparse node set (BUILD-TIME only; this is
  // NOT a navmesh and must not grow per-tick scanning). Edges emit in ascending (from, to) order.
  const SpatialSurfaceSet surfaces = buildSpatialSurfaceSet(room);
  PhysicsSpatialSurfaceColliderBakeRequest bakeRequest;
  bakeRequest.surfaces = &surfaces;
  const PhysicsSpatialSurfaceColliderBakeResult bake =
      bakePhysicsAabbCollidersFromSpatialSurfaces(bakeRequest);
  if (!bake.ok) {
    return graph;
  }
  const std::vector<PhysicsAabbCollider>& colliders = bake.colliders;

  for (std::size_t i = 0; i < graph.nodes.size(); ++i) {
    for (std::size_t j = i + 1; j < graph.nodes.size(); ++j) {
      const Vec3 a = graph.nodes[i].positionMeters;
      const Vec3 b = graph.nodes[j].positionMeters;
      const float len = std::sqrt(lengthSquared(b - a));
      if (len > config.maxLinkDistanceMeters) {
        continue;
      }
      if (reasoningSegmentBlocked(colliders,
                                  a,
                                  b,
                                  config.occlusionEyeHeightMeters,
                                  config.occlusionMarginMeters)) {
        continue;
      }
      graph.edges.push_back(ReasoningEdge{graph.nodes[i].id, graph.nodes[j].id,
                                          ReasoningEdgeKind::walkable, len});
    }
  }
  return graph;
}

}  // namespace iggy3d
