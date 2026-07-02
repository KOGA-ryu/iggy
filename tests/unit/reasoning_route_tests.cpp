// a4s1 — L5 travel cost + routing over the L4 reasoning graph: pure, headless unit tests over
// hand-built mini-graphs. Straight chain, cost-beats-hop-count fork, id tie-break + determinism,
// blocked entry (empty), disconnected components (empty), and edge-kind multiplier steering. The
// GARDEN pin (real a3s1 geometry) lives in stealth_garden_tests. Nothing consumes planRoute ->
// zero behavior change.

#include "runtime/ai/ReasoningRoute.hpp"

#include "content/assets/RoomAsset.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/ai/ReasoningGraph.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/physics/PhysicsAabbCollider.hpp"
#include "runtime/physics/PhysicsSpatialSurfaceColliderBake.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::ReasoningNode node(std::uint32_t id, iggy3d::Vec3 pos) {
  iggy3d::ReasoningNode n;
  n.id = id;
  n.kind = iggy3d::ReasoningNodeKind::reference;
  n.positionMeters = pos;
  n.sourceLabel = "test";
  return n;
}

iggy3d::ReasoningEdge edge(std::uint32_t from, std::uint32_t to, float lengthMeters,
                           iggy3d::ReasoningEdgeKind kind = iggy3d::ReasoningEdgeKind::walkable) {
  iggy3d::ReasoningEdge e;
  e.from = from;
  e.to = to;
  e.kind = kind;
  e.lengthMeters = lengthMeters;
  return e;
}

// An axis box blocker spanning [x0,x1] x [0,3] x [z0,z1], baked to colliders the shared segment
// discipline registers (same as the garden '#' cells).
std::vector<iggy3d::PhysicsAabbCollider> bakeWall(float x0, float x1, float z0, float z1) {
  iggy3d::RoomAsset room;
  room.id = "route_wall";
  iggy3d::RoomSpatialSurface wall;
  wall.id = "wall";
  wall.sourceStaticMeshId = "wall";
  wall.shape = iggy3d::RoomSpatialSurfaceShape::Box;
  wall.role = iggy3d::RoomSpatialSurfaceRole::Blocker;
  wall.pointsMeters = {{x0, 0.0F, z0}, {x1, 0.0F, z0}, {x1, 3.0F, z1}, {x0, 3.0F, z1}};
  wall.normal = {0.0F, 0.0F, 1.0F};
  wall.collisionMask = {"actor"};
  wall.blocksActor = true;
  room.spatialSurfaces.push_back(std::move(wall));

  const iggy3d::SpatialSurfaceSet surfaces = iggy3d::buildSpatialSurfaceSet(room);
  iggy3d::PhysicsSpatialSurfaceColliderBakeRequest request;
  request.surfaces = &surfaces;
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult bake =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces(request);
  return bake.ok ? bake.colliders : std::vector<iggy3d::PhysicsAabbCollider>{};
}

bool routeEquals(const iggy3d::PlannedRoute& route, const std::vector<std::uint32_t>& expected) {
  return route.nodeIds == expected;
}

bool straightChainRoutesEndToEnd() {
  iggy3d::ReasoningGraph g;
  g.nodes = {node(0, {0, 0, 0}), node(1, {3, 0, 0}), node(2, {6, 0, 0}), node(3, {9, 0, 0})};
  g.edges = {edge(0, 1, 3), edge(1, 2, 3), edge(2, 3, 3)};
  const std::vector<iggy3d::PhysicsAabbCollider> noColliders;
  const iggy3d::PlannedRoute route =
      iggy3d::planRoute(g, noColliders, {0, 0, 0}, {9, 0, 0});
  return expect(routeEquals(route, {0, 1, 2, 3}), "straight chain routes end to end");
}

bool cheaperLongPathBeatsHopCount() {
  iggy3d::ReasoningGraph g;
  g.nodes = {node(0, {0, 0, 0}), node(1, {2, 0, 0}), node(2, {0, 0, 5})};
  // Direct short hop 0->1 is EXPENSIVE (dangerous x10 = 20); the 2-hop detour is cheaper (5+5=10).
  g.edges = {edge(0, 1, 2, iggy3d::ReasoningEdgeKind::dangerous), edge(0, 2, 5), edge(2, 1, 5)};
  iggy3d::TravelCostConfig config;
  config.edgeKindMultipliers[static_cast<std::size_t>(iggy3d::ReasoningEdgeKind::dangerous)] = 10.0F;
  const std::vector<iggy3d::PhysicsAabbCollider> noColliders;
  const iggy3d::PlannedRoute route =
      iggy3d::planRoute(g, noColliders, {0, 0, 0}, {2, 0, 0}, config);
  return expect(routeEquals(route, {0, 2, 1}), "cheaper long path beats the shorter hop count");
}

bool tieBreakPrefersLowerIdPathAndIsDeterministic() {
  iggy3d::ReasoningGraph g;
  g.nodes = {node(0, {0, 0, 0}), node(1, {5, 0, 3}), node(2, {5, 0, -3}), node(3, {10, 0, 0})};
  g.edges = {edge(0, 1, 5), edge(0, 2, 5), edge(1, 3, 5), edge(2, 3, 5)};  // two equal-cost paths
  const std::vector<iggy3d::PhysicsAabbCollider> noColliders;
  const iggy3d::PlannedRoute a = iggy3d::planRoute(g, noColliders, {0, 0, 0}, {10, 0, 0});
  const iggy3d::PlannedRoute b = iggy3d::planRoute(g, noColliders, {0, 0, 0}, {10, 0, 0});
  return expect(routeEquals(a, {0, 1, 3}), "equal-cost tie resolves to the lower-id path") &&
         expect(a.nodeIds == b.nodeIds, "identical inputs yield a bitwise-identical route");
}

bool blockedEntryYieldsEmptyRoute() {
  iggy3d::ReasoningGraph g;
  g.nodes = {node(0, {5, 0, 0}), node(1, {5, 0, 3})};
  g.edges = {edge(0, 1, 3)};
  // A wall at x~2.5 spanning z[-1,4] sits between `from` (x=0) and every node (x=5).
  const std::vector<iggy3d::PhysicsAabbCollider> wall = bakeWall(2.0F, 3.0F, -1.0F, 4.0F);
  const iggy3d::PlannedRoute route =
      iggy3d::planRoute(g, wall, {0, 0, 0}, {5, 0, 1.5F});
  return expect(!wall.empty(), "wall colliders baked") &&
         expect(route.nodeIds.empty(), "no reachable entry node -> empty route (caller falls back)");
}

bool disconnectedComponentsYieldEmptyRoute() {
  iggy3d::ReasoningGraph g;
  g.nodes = {node(0, {0, 0, 0}), node(1, {2, 0, 0}), node(2, {20, 0, 0}), node(3, {22, 0, 0})};
  g.edges = {edge(0, 1, 2), edge(2, 3, 2)};  // two components, no link between
  const std::vector<iggy3d::PhysicsAabbCollider> noColliders;
  const iggy3d::PlannedRoute route =
      iggy3d::planRoute(g, noColliders, {0, 0, 0}, {20, 0, 0});
  return expect(route.nodeIds.empty(), "exit in a disconnected component -> empty route");
}

bool edgeKindMultiplierSteersAway() {
  iggy3d::ReasoningGraph g;
  g.nodes = {node(0, {0, 0, 0}), node(1, {4, 0, 0}), node(2, {2, 0, 3})};
  g.edges = {edge(0, 1, 3, iggy3d::ReasoningEdgeKind::noisy), edge(0, 2, 2), edge(2, 1, 2)};
  const std::vector<iggy3d::PhysicsAabbCollider> noColliders;

  // Neutral multipliers: the direct noisy hop (len 3) beats the detour (2+2=4).
  const iggy3d::PlannedRoute cheap = iggy3d::planRoute(g, noColliders, {0, 0, 0}, {4, 0, 0});
  bool ok = expect(routeEquals(cheap, {0, 1}), "neutral cost takes the direct edge");

  // Make `noisy` expensive (x10 = 30): the route now flanks via the detour.
  iggy3d::TravelCostConfig config;
  config.edgeKindMultipliers[static_cast<std::size_t>(iggy3d::ReasoningEdgeKind::noisy)] = 10.0F;
  const iggy3d::PlannedRoute steered = iggy3d::planRoute(g, noColliders, {0, 0, 0}, {4, 0, 0}, config);
  ok = ok && expect(routeEquals(steered, {0, 2, 1}), "an expensive edge kind steers the route away");
  return ok;
}

bool travelCostAppliesLengthAndMultiplier() {
  const iggy3d::ReasoningEdge walk = edge(0, 1, 4.0F, iggy3d::ReasoningEdgeKind::walkable);
  const iggy3d::ReasoningEdge danger = edge(0, 1, 4.0F, iggy3d::ReasoningEdgeKind::dangerous);
  iggy3d::TravelCostConfig config;
  config.edgeKindMultipliers[static_cast<std::size_t>(iggy3d::ReasoningEdgeKind::dangerous)] = 2.5F;
  return expect(iggy3d::travelCost(walk, {}) == 4.0F, "walkable cost = length x 1.0") &&
         expect(iggy3d::travelCost(danger, config) == 10.0F, "dangerous cost = length x multiplier");
}

}  // namespace

int main() {
  const bool ok = straightChainRoutesEndToEnd() && cheaperLongPathBeatsHopCount() &&
                  tieBreakPrefersLowerIdPathAndIsDeterministic() && blockedEntryYieldsEmptyRoute() &&
                  disconnectedComponentsYieldEmptyRoute() && edgeKindMultiplierSteersAway() &&
                  travelCostAppliesLengthAndMultiplier();
  return ok ? 0 : 1;
}
