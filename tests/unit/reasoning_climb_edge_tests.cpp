// MA4 s1 GATE 2 -- climb-edge emission + class-aware routing. A two-node fixture split by a blocking
// wall a traversal slot bridges: the walkable edge is ABSENT, a climb edge is PRESENT; a grounded
// guard can't route it, a climber routes THROUGH it. Empty slots => the graph is byte-identical.

#include "runtime/ai/ReasoningGraph.hpp"
#include "runtime/ai/ReasoningRoute.hpp"

#include <cmath>
#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "content/assets/RoomAsset.hpp"
#include "core/math/Aabb3.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/movement/MovementCapability.hpp"
#include "runtime/movement/MovementTraversalSlots.hpp"

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

// A y:0..3 box blocker cell centered on (cx, cz) -- registers on the eye-height (y=1) segment ray.
iggy3d::RoomSpatialSurface boxWall(const std::string& id, float cx, float cz) {
  iggy3d::RoomSpatialSurface wall;
  wall.id = id;
  wall.sourceStaticMeshId = id;
  wall.shape = iggy3d::RoomSpatialSurfaceShape::Box;
  wall.role = iggy3d::RoomSpatialSurfaceRole::Blocker;
  wall.pointsMeters = {{cx - 0.5F, 0.0F, cz - 0.5F},
                       {cx + 0.5F, 0.0F, cz - 0.5F},
                       {cx + 0.5F, 3.0F, cz + 0.5F},
                       {cx - 0.5F, 3.0F, cz + 0.5F}};
  wall.normal = {0.0F, 0.0F, 1.0F};
  wall.collisionMask = {"actor"};
  wall.blocksActor = true;
  return wall;
}

// A clamber slot that bridges a node at `frontNodeSide` with a node at the landing side: front face
// centered at `front`, landing at `landing`, the slot body (targetBounds) at `body`.
iggy3d::MovementTraversalSlot bridgeSlot(const std::string& id, iggy3d::Vec3 front, iggy3d::Vec3 body,
                                         iggy3d::Vec3 landing) {
  iggy3d::MovementTraversalSlot slot;
  slot.slotId = id;
  slot.kind = iggy3d::MovementTraversalSlotKind::Clamber;
  slot.frontFaceBounds = iggy3d::makeAabb3(front, front);
  slot.targetBounds = iggy3d::makeAabb3(body, body);
  slot.landingBounds = iggy3d::makeAabb3(landing, landing);
  slot.landingPosition = landing;
  return slot;
}

bool nearly(float a, float b) { return std::fabs(a - b) < 1e-3F; }

// Two nodes at (0,0,0) and (0,0,4), a wall at z=2 blocking the direct segment, a slot bridging them.
iggy3d::RoomAsset twoNodeBlockedRoom() {
  iggy3d::RoomAsset room;
  room.id = "climb_emit_test";
  room.anchors = {anchor("spawn", {0.0F, 0.0F, 0.0F}), anchor("npc", {0.0F, 0.0F, 4.0F})};
  room.spatialSurfaces = {boxWall("wall_mid", 0.0F, 2.0F)};
  return room;
}

bool climbEdgeEmittedWhereSlotBridgesBlockedPair() {
  const iggy3d::RoomAsset room = twoNodeBlockedRoom();
  const std::vector<iggy3d::Vec3> noWaypoints;
  const std::vector<iggy3d::MovementTraversalSlot> slots = {
      bridgeSlot("climb_a_b", {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F, 2.0F}, {0.0F, 0.0F, 3.0F})};

  // With slots: no walkable edge (blocked), exactly one climb edge, length = |a->body| + |body->b|.
  const iggy3d::ReasoningGraph withSlots =
      iggy3d::buildReasoningGraph(room, noWaypoints, {}, slots);
  bool ok = expect(withSlots.nodes.size() == 2U, "two nodes present") &&
            expect(withSlots.edges.size() == 1U, "exactly one edge (the climb bridge)") &&
            expect(!withSlots.edges.empty() &&
                       withSlots.edges[0].kind == iggy3d::ReasoningEdgeKind::climb,
                   "the emitted edge is a climb edge") &&
            expect(!withSlots.edges.empty() && nearly(withSlots.edges[0].lengthMeters, 4.0F),
                   "climb length = through-slot path (2 + 2 = 4 m)");

  // NO slots => byte-identical to the pre-MA4 graph (blocked pair => empty edge list).
  const iggy3d::ReasoningGraph noSlots = iggy3d::buildReasoningGraph(room, noWaypoints);
  ok = ok && expect(noSlots.edges.empty(),
                    "no slots => the blocked pair yields NO edge (graph byte-identical)");
  return ok;
}

bool distanceOnlyAbsentPairGetsNoClimbEdge() {
  iggy3d::RoomAsset room;
  room.id = "far_pair_test";
  // 30 m apart -- beyond the 20 m max link. No wall (not blocked); the pair is absent by DISTANCE.
  room.anchors = {anchor("spawn", {0.0F, 0.0F, 0.0F}), anchor("npc", {0.0F, 0.0F, 30.0F})};
  const std::vector<iggy3d::Vec3> noWaypoints;
  const std::vector<iggy3d::MovementTraversalSlot> slots = {
      bridgeSlot("climb_far", {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F, 15.0F}, {0.0F, 0.0F, 29.0F})};
  const iggy3d::ReasoningGraph graph = iggy3d::buildReasoningGraph(room, noWaypoints, {}, slots);
  return expect(graph.edges.empty(),
                "a distance-only-absent pair gets NO climb edge (never reaches the blocked branch)");
}

bool climberRoutesThroughGroundedDoesNot() {
  const iggy3d::RoomAsset room = twoNodeBlockedRoom();
  const std::vector<iggy3d::Vec3> noWaypoints;
  const std::vector<iggy3d::MovementTraversalSlot> slots = {
      bridgeSlot("climb_a_b", {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F, 2.0F}, {0.0F, 0.0F, 3.0F})};
  const iggy3d::ReasoningGraph graph = iggy3d::buildReasoningGraph(room, noWaypoints, {}, slots);
  const std::vector<iggy3d::PhysicsAabbCollider> noColliders;

  const iggy3d::PlannedRoute grounded =
      iggy3d::planRoute(graph, noColliders, {0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 4.0F},
                        iggy3d::travelCostConfigForCapability(iggy3d::MovementCapabilityClass::grounded));
  const iggy3d::PlannedRoute climber =
      iggy3d::planRoute(graph, noColliders, {0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 4.0F},
                        iggy3d::travelCostConfigForCapability(iggy3d::MovementCapabilityClass::climber));

  return expect(grounded.nodeIds.empty(),
                "grounded can't use the climb edge -> no route (walks around; none here)") &&
         expect(climber.nodeIds.size() == 2U, "climber routes THROUGH the climb edge") &&
         expect(climber.totalCostMeters > 0.0F, "climber cost is positive") &&
         expect(std::isfinite(climber.totalCostMeters), "climber cost is finite (sentinel skipped)") &&
         expect(climber.totalCostMeters >= 0.0F, "no negative sentinel leaked into the cost");
}

bool climberBeatsGroundedDetourRelatively() {
  // Add a detour node C off to the side so grounded has a LONGER walkable path A->C->B while the
  // climb edge A->B is short. totalCost asserted RELATIVELY -- no literal pins.
  iggy3d::RoomAsset room;
  room.id = "detour_test";
  room.anchors = {anchor("spawn", {0.0F, 0.0F, 0.0F}), anchor("npc", {0.0F, 0.0F, 4.0F}),
                  anchor("exit", {6.0F, 0.0F, 2.0F})};
  room.spatialSurfaces = {boxWall("wall_mid", 0.0F, 2.0F)};  // blocks A-B only (C is off to the side)
  const std::vector<iggy3d::Vec3> noWaypoints;
  const std::vector<iggy3d::MovementTraversalSlot> slots = {
      bridgeSlot("climb_a_b", {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F, 2.0F}, {0.0F, 0.0F, 3.0F})};
  const iggy3d::ReasoningGraph graph = iggy3d::buildReasoningGraph(room, noWaypoints, {}, slots);
  const std::vector<iggy3d::PhysicsAabbCollider> noColliders;

  const iggy3d::PlannedRoute grounded =
      iggy3d::planRoute(graph, noColliders, {0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 4.0F},
                        iggy3d::travelCostConfigForCapability(iggy3d::MovementCapabilityClass::grounded));
  const iggy3d::PlannedRoute climber =
      iggy3d::planRoute(graph, noColliders, {0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 4.0F},
                        iggy3d::travelCostConfigForCapability(iggy3d::MovementCapabilityClass::climber));

  return expect(!grounded.nodeIds.empty(), "grounded takes the walkable detour A->C->B") &&
         expect(!climber.nodeIds.empty(), "climber routes A->B") &&
         expect(grounded.totalCostMeters > 0.0F && climber.totalCostMeters > 0.0F,
                "both routes have positive cost") &&
         expect(climber.totalCostMeters < grounded.totalCostMeters,
                "the climb route is cheaper than the grounded detour (relative, no literal pin)");
}

bool emissionIsDeterministic() {
  const iggy3d::RoomAsset room = twoNodeBlockedRoom();
  const std::vector<iggy3d::Vec3> noWaypoints;
  const std::vector<iggy3d::MovementTraversalSlot> slots = {
      bridgeSlot("climb_a_b", {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F, 2.0F}, {0.0F, 0.0F, 3.0F})};
  const iggy3d::ReasoningGraph a = iggy3d::buildReasoningGraph(room, noWaypoints, {}, slots);
  const iggy3d::ReasoningGraph b = iggy3d::buildReasoningGraph(room, noWaypoints, {}, slots);
  bool ok = expect(a.edges.size() == b.edges.size(), "same edge count across builds");
  for (std::size_t i = 0; ok && i < a.edges.size(); ++i) {
    ok = ok && expect(a.edges[i].from == b.edges[i].from && a.edges[i].to == b.edges[i].to &&
                          a.edges[i].kind == b.edges[i].kind &&
                          a.edges[i].lengthMeters == b.edges[i].lengthMeters,
                      "edge list is bitwise-identical across builds");
  }
  return ok;
}

}  // namespace

int main() {
  const bool ok = climbEdgeEmittedWhereSlotBridgesBlockedPair() &&
                  distanceOnlyAbsentPairGetsNoClimbEdge() && climberRoutesThroughGroundedDoesNot() &&
                  climberBeatsGroundedDetourRelatively() && emissionIsDeterministic();
  return ok ? 0 : 1;
}
