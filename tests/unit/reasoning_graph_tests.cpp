// a3s1 — the L4 reasoning graph kernel: pure unit tests over hand-built RoomAssets. Determinism,
// anchor->kind mapping, waypoint->patrolPost, a wall blocking a walkable edge (negative), and the
// named max-link-distance cutoff. The GARDEN SHAPE pin lives in stealth_garden_tests (it reuses
// that harness's loadGarden). Nothing consumes the graph yet -> zero behavior change.

#include "runtime/ai/ReasoningGraph.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "content/assets/RoomAsset.hpp"
#include "core/math/Vec3.hpp"

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::RoomAnchorAsset anchor(const std::string& kind, iggy3d::Vec3 pos) {
  iggy3d::RoomAnchorAsset a;
  a.id = "anchor_" + kind;
  a.kind = kind;
  a.positionMeters = pos;
  return a;
}

// A y:0..3 box blocker cell centered on (cx, cz) -- the same shape the garden bakes for a '#'
// wall, so the eye-height (y=1) segment ray registers a hit.
iggy3d::RoomSpatialSurface boxWall(const std::string& id, float cx, float cz) {
  iggy3d::RoomSpatialSurface wall;
  wall.id = id;
  wall.sourceStaticMeshId = id;
  wall.shape = iggy3d::RoomSpatialSurfaceShape::Box;
  wall.role = iggy3d::RoomSpatialSurfaceRole::Blocker;
  const float x0 = cx - 0.5F;
  const float x1 = cx + 0.5F;
  const float z0 = cz - 0.5F;
  const float z1 = cz + 0.5F;
  wall.pointsMeters = {{x0, 0.0F, z0}, {x1, 0.0F, z0}, {x1, 3.0F, z1}, {x0, 3.0F, z1}};
  wall.normal = {0.0F, 0.0F, 1.0F};
  wall.collisionMask = {"actor"};
  wall.blocksActor = true;
  return wall;
}

bool vec3Exact(iggy3d::Vec3 a, iggy3d::Vec3 b) {
  return a.x == b.x && a.y == b.y && a.z == b.z;
}

bool anchorMappingDerivesRightKinds() {
  iggy3d::RoomAsset room;
  room.id = "map_test";
  room.anchors = {
      anchor("exit", {1.0F, 0.0F, 1.0F}),
      anchor("treasure", {2.0F, 0.0F, 2.0F}),
      anchor("key", {3.0F, 0.0F, 3.0F}),
      anchor("pickup", {4.0F, 0.0F, 4.0F}),
      anchor("spawn", {5.0F, 0.0F, 5.0F}),
      anchor("npc", {6.0F, 0.0F, 6.0F}),
      anchor("door", {7.0F, 0.0F, 7.0F}),        // not derived in v1 (doorway is a3s1b)
      anchor("marker", {8.0F, 0.0F, 8.0F}),      // not derived in v1
      anchor("reset_zone", {9.0F, 0.0F, 9.0F}),  // not derived in v1
  };
  const std::vector<iggy3d::Vec3> noWaypoints;
  const iggy3d::ReasoningGraph g = iggy3d::buildReasoningGraph(room, noWaypoints);

  int exits = 0;
  int objectives = 0;
  int references = 0;
  int other = 0;
  for (const iggy3d::ReasoningNode& n : g.nodes) {
    switch (n.kind) {
      case iggy3d::ReasoningNodeKind::exit: ++exits; break;
      case iggy3d::ReasoningNodeKind::objective: ++objectives; break;
      case iggy3d::ReasoningNodeKind::reference: ++references; break;
      default: ++other; break;
    }
  }
  return expect(g.nodes.size() == 6U, "only the 6 derivable anchors become nodes") &&
         expect(exits == 1, "one exit node") &&
         expect(objectives == 3, "treasure+key+pickup -> three objective nodes") &&
         expect(references == 2, "spawn+npc -> two reference nodes") &&
         expect(other == 0, "door/marker/reset_zone derive no node in v1");
}

bool waypointsBecomePatrolPosts() {
  iggy3d::RoomAsset room;
  room.id = "waypoint_test";
  const std::vector<iggy3d::Vec3> waypoints = {{1.0F, 0.0F, 1.0F}, {2.0F, 0.0F, 5.0F}};
  const iggy3d::ReasoningGraph g = iggy3d::buildReasoningGraph(room, waypoints);

  int posts = 0;
  bool provenance = true;
  for (const iggy3d::ReasoningNode& n : g.nodes) {
    if (n.kind == iggy3d::ReasoningNodeKind::patrolPost) {
      ++posts;
      provenance = provenance && (n.sourceLabel == "waypoint");
    }
  }
  return expect(g.nodes.size() == 2U, "two waypoints -> two nodes") &&
         expect(posts == 2, "both nodes are patrolPost") &&
         expect(provenance, "patrolPost provenance is 'waypoint'");
}

bool wallBlocksWalkableEdge() {
  iggy3d::RoomAsset room;
  room.id = "blocked_test";
  room.anchors = {anchor("spawn", {0.0F, 0.0F, 0.0F}), anchor("npc", {0.0F, 0.0F, 4.0F})};
  const std::vector<iggy3d::Vec3> noWaypoints;

  // A wall cell at z=2 sits directly on the segment between the two nodes.
  room.spatialSurfaces = {boxWall("wall_mid", 0.0F, 2.0F)};
  const iggy3d::ReasoningGraph blocked = iggy3d::buildReasoningGraph(room, noWaypoints);
  bool ok = expect(blocked.nodes.size() == 2U, "two nodes present") &&
            expect(blocked.edges.empty(), "wall between the nodes blocks the walkable edge");

  // Remove the wall and the edge appears (positive control).
  room.spatialSurfaces.clear();
  const iggy3d::ReasoningGraph clear = iggy3d::buildReasoningGraph(room, noWaypoints);
  ok = ok && expect(clear.edges.size() == 1U, "removing the wall restores the walkable edge") &&
       expect(clear.edges.empty() ? false : clear.edges[0].kind == iggy3d::ReasoningEdgeKind::walkable,
              "the restored edge is walkable");
  return ok;
}

bool maxLinkDistanceCutsLongEdges() {
  iggy3d::RoomAsset room;
  room.id = "cutoff_test";
  room.anchors = {anchor("spawn", {0.0F, 0.0F, 0.0F}), anchor("npc", {0.0F, 0.0F, 30.0F})};
  const std::vector<iggy3d::Vec3> noWaypoints;

  const iggy3d::ReasoningGraph atDefault = iggy3d::buildReasoningGraph(room, noWaypoints);
  bool ok = expect(atDefault.edges.empty(), "30 m pair exceeds the default 20 m link cutoff");

  iggy3d::ReasoningGraphConfig wide;
  wide.maxLinkDistanceMeters = 40.0F;
  const iggy3d::ReasoningGraph widened = iggy3d::buildReasoningGraph(room, noWaypoints, wide);
  ok = ok && expect(widened.edges.size() == 1U, "raising the cutoff to 40 m links the pair");
  return ok;
}

bool sameInputsYieldBitwiseIdenticalGraph() {
  iggy3d::RoomAsset room;
  room.id = "determinism_test";
  room.anchors = {
      anchor("exit", {12.0F, 0.0F, 1.0F}),
      anchor("npc", {1.0F, 0.0F, 1.0F}),
      anchor("spawn", {12.0F, 0.0F, 6.0F}),
      anchor("treasure", {6.0F, 0.0F, 3.0F}),
  };
  const std::vector<iggy3d::Vec3> waypoints = {{1.0F, 0.0F, 1.0F}, {1.0F, 0.0F, 5.0F}};
  const iggy3d::ReasoningGraph a = iggy3d::buildReasoningGraph(room, waypoints);
  const iggy3d::ReasoningGraph b = iggy3d::buildReasoningGraph(room, waypoints);

  bool ok = expect(a.nodes.size() == b.nodes.size() && a.edges.size() == b.edges.size(),
                   "graph sizes match across builds");
  if (!ok) {
    return false;
  }
  // Stable ordering: ids are the sorted index, so kinds are non-decreasing across the node list.
  bool ordered = true;
  for (std::size_t i = 0; i < a.nodes.size(); ++i) {
    ordered = ordered && (a.nodes[i].id == static_cast<std::uint32_t>(i));
    if (i > 0) {
      ordered = ordered &&
                (static_cast<std::uint8_t>(a.nodes[i - 1].kind) <=
                 static_cast<std::uint8_t>(a.nodes[i].kind));
    }
    ok = ok && a.nodes[i].id == b.nodes[i].id && a.nodes[i].kind == b.nodes[i].kind &&
         vec3Exact(a.nodes[i].positionMeters, b.nodes[i].positionMeters) &&
         a.nodes[i].sourceLabel == b.nodes[i].sourceLabel;
  }
  for (std::size_t i = 0; i < a.edges.size(); ++i) {
    ok = ok && a.edges[i].from == b.edges[i].from && a.edges[i].to == b.edges[i].to &&
         a.edges[i].kind == b.edges[i].kind && a.edges[i].lengthMeters == b.edges[i].lengthMeters;
  }
  return expect(ok, "identical inputs yield a bitwise-identical graph") &&
         expect(ordered, "ids are the stable-sorted index (kinds non-decreasing)");
}

bool summaryCountsMatchGraph() {
  iggy3d::RoomAsset room;
  room.id = "summary_test";
  room.anchors = {
      anchor("exit", {1.0F, 0.0F, 1.0F}),
      anchor("treasure", {2.0F, 0.0F, 2.0F}),
      anchor("key", {3.0F, 0.0F, 3.0F}),
      anchor("spawn", {4.0F, 0.0F, 4.0F}),
      anchor("npc", {5.0F, 0.0F, 5.0F}),
  };
  const std::vector<iggy3d::Vec3> waypoints = {{6.0F, 0.0F, 6.0F}};
  const iggy3d::ReasoningGraph g = iggy3d::buildReasoningGraph(room, waypoints);
  const iggy3d::ReasoningGraphSummary s = iggy3d::summarizeReasoningGraph(g);

  const auto kindCount = [&s](iggy3d::ReasoningNodeKind k) {
    return s.perKindCounts[static_cast<std::size_t>(k)];
  };
  std::size_t perKindTotal = 0;
  for (std::size_t c : s.perKindCounts) {
    perKindTotal += c;
  }
  return expect(s.nodeCount == g.nodes.size(), "summary node count matches the graph") &&
         expect(s.edgeCount == g.edges.size(), "summary edge count matches the graph") &&
         expect(kindCount(iggy3d::ReasoningNodeKind::exit) == 1U, "summary: 1 exit") &&
         expect(kindCount(iggy3d::ReasoningNodeKind::objective) == 2U, "summary: 2 objective") &&
         expect(kindCount(iggy3d::ReasoningNodeKind::reference) == 2U, "summary: 2 reference") &&
         expect(kindCount(iggy3d::ReasoningNodeKind::patrolPost) == 1U, "summary: 1 patrolPost") &&
         expect(perKindTotal == s.nodeCount, "per-kind counts sum to the node count");
}

}  // namespace

int main() {
  const bool ok = anchorMappingDerivesRightKinds() && waypointsBecomePatrolPosts() &&
                  wallBlocksWalkableEdge() && maxLinkDistanceCutsLongEdges() &&
                  sameInputsYieldBitwiseIdenticalGraph() && summaryCountsMatchGraph();
  return ok ? 0 : 1;
}
