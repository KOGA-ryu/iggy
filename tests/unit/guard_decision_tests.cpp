// a5s1 — L6 guard-decision kernel: pure, headless unit tests over hand-built mini-graphs.
// Determinism + tie-break, factor isolation (suspicion / strategic / travel / personality-weight
// flip), unreachable exclusion, suppression, cold-memory decay, and the none-with-receipt cases.
// The ONE config-coupled garden pin lives in stealth_garden_tests (quarantine law). Nothing
// consumes chooseSearchNode -> zero behavior change.

#include "runtime/ai/GuardDecision.hpp"

#include "content/assets/RoomAsset.hpp"
#include "core/ids/EntityId.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/ai/NpcPersonalityWeights.hpp"
#include "runtime/ai/ReasoningGraph.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/physics/PhysicsAabbCollider.hpp"
#include "runtime/physics/PhysicsSpatialSurfaceColliderBake.hpp"

#include <cstdint>
#include <iostream>
#include <optional>
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

iggy3d::ReasoningNode node(std::uint32_t id, iggy3d::ReasoningNodeKind kind, iggy3d::Vec3 pos) {
  iggy3d::ReasoningNode n;
  n.id = id;
  n.kind = kind;
  n.positionMeters = pos;
  n.sourceLabel = "test";
  return n;
}

iggy3d::ReasoningEdge edge(std::uint32_t from, std::uint32_t to, float lengthMeters) {
  iggy3d::ReasoningEdge e;
  e.from = from;
  e.to = to;
  e.kind = iggy3d::ReasoningEdgeKind::walkable;
  e.lengthMeters = lengthMeters;
  return e;
}

std::vector<iggy3d::PhysicsAabbCollider> bakeWall(float x0, float x1, float z0, float z1) {
  iggy3d::RoomAsset room;
  room.id = "decision_wall";
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

float factorValue(const iggy3d::GuardDecisionReceipt& receipt, std::string_view name) {
  for (const iggy3d::GuardDecisionFactor& f : receipt.factors) {
    if (f.name == name) {
      return f.value;
    }
  }
  return 0.0F;
}

const std::vector<iggy3d::PhysicsAabbCollider> kNoColliders;
const iggy3d::NpcPersonalityWeights kNeutral;
const iggy3d::GuardDecisionConfig kConfig;

bool determinismAndTieBreak() {
  iggy3d::ReasoningGraph g;
  // Two co-located, same-kind nodes -> identical score -> the lower id must win.
  g.nodes = {node(0, iggy3d::ReasoningNodeKind::reference, {5, 0, 0}),
             node(1, iggy3d::ReasoningNodeKind::reference, {5, 0, 0})};
  const iggy3d::GuardMemorySample noMemory;
  const iggy3d::GuardDecision a =
      iggy3d::chooseSearchNode(g, kNoColliders, {0, 0, 0}, noMemory, 0, iggy3d::EntityId{},
                               kNeutral, kConfig);
  const iggy3d::GuardDecision b =
      iggy3d::chooseSearchNode(g, kNoColliders, {0, 0, 0}, noMemory, 0, iggy3d::EntityId{},
                               kNeutral, kConfig);
  bool ok = expect(a.nodeId.has_value() && *a.nodeId == 0U, "co-located tie resolves to the lower id");
  ok = ok && expect(a.receipt.chosenNodeId == b.receipt.chosenNodeId &&
                        a.receipt.totalScore == b.receipt.totalScore &&
                        a.receipt.factors.size() == b.receipt.factors.size(),
                    "identical inputs yield a bitwise-identical receipt");
  for (std::size_t i = 0; ok && i < a.receipt.factors.size(); ++i) {
    ok = ok && a.receipt.factors[i].name == b.receipt.factors[i].name &&
         a.receipt.factors[i].value == b.receipt.factors[i].value;
  }
  return expect(ok, "receipt determinism holds across factors");
}

bool memoryProximityDominatesNearLastKnown() {
  iggy3d::ReasoningGraph g;
  // Three same-kind nodes, all 5 m from the guard (equal travel + strategic); memory hugs node 0.
  g.nodes = {node(0, iggy3d::ReasoningNodeKind::patrolPost, {5, 0, 0}),
             node(1, iggy3d::ReasoningNodeKind::patrolPost, {-5, 0, 0}),
             node(2, iggy3d::ReasoningNodeKind::patrolPost, {0, 0, 5})};
  iggy3d::GuardMemorySample memory;
  memory.hasMemorySample = true;
  memory.lastKnownPosition = {5, 0, 0};
  memory.lastKnownTick = 0;
  const iggy3d::GuardDecision d = iggy3d::chooseSearchNode(g, kNoColliders, {0, 0, 0}, memory, 0,
                                                           iggy3d::EntityId{}, kNeutral, kConfig);
  return expect(d.nodeId.has_value() && *d.nodeId == 0U,
                "the node nearest the last-known wins on suspicion") &&
         expect(factorValue(d.receipt, "suspicion") > 0.0F, "suspicion is positive at the memory");
}

bool strategicSteersWhenSuspicionFlat() {
  iggy3d::ReasoningGraph g;
  g.nodes = {node(0, iggy3d::ReasoningNodeKind::patrolPost, {5, 0, 0}),
             node(1, iggy3d::ReasoningNodeKind::exit, {-5, 0, 0}),
             node(2, iggy3d::ReasoningNodeKind::reference, {0, 0, 5})};
  const iggy3d::GuardMemorySample noMemory;  // suspicion flat (0) everywhere
  const iggy3d::GuardDecision d = iggy3d::chooseSearchNode(g, kNoColliders, {0, 0, 0}, noMemory, 0,
                                                           iggy3d::EntityId{}, kNeutral, kConfig);
  return expect(d.nodeId.has_value() && *d.nodeId == 1U,
                "with flat suspicion the highest strategic kind (exit) wins") &&
         expect(factorValue(d.receipt, "suspicion") == 0.0F, "no memory -> suspicion 0");
}

bool travelPenalizesFarNode() {
  iggy3d::ReasoningGraph g;
  g.nodes = {node(0, iggy3d::ReasoningNodeKind::exit, {3, 0, 0}),
             node(1, iggy3d::ReasoningNodeKind::exit, {10, 0, 0})};
  const iggy3d::GuardMemorySample noMemory;
  const iggy3d::GuardDecision d = iggy3d::chooseSearchNode(g, kNoColliders, {0, 0, 0}, noMemory, 0,
                                                           iggy3d::EntityId{}, kNeutral, kConfig);
  return expect(d.nodeId.has_value() && *d.nodeId == 0U,
                "equal-kind nodes: the nearer one wins on travel") &&
         expect(factorValue(d.receipt, "travel") < 0.0F, "travel is a negative contribution");
}

bool personalityWeightFlipsChoice() {
  iggy3d::ReasoningGraph g;
  g.nodes = {node(0, iggy3d::ReasoningNodeKind::patrolPost, {3, 0, 0}),
             node(1, iggy3d::ReasoningNodeKind::exit, {10, 0, 0})};
  const iggy3d::GuardMemorySample noMemory;
  // Neutral: the far exit (strat 20 - 10) beats the near patrolPost (2 - 3).
  const iggy3d::GuardDecision neutral = iggy3d::chooseSearchNode(
      g, kNoColliders, {0, 0, 0}, noMemory, 0, iggy3d::EntityId{}, kNeutral, kConfig);
  // A guard that HATES travelling (travelWeight x3) flips to the near node.
  iggy3d::NpcPersonalityWeights homebody;
  homebody.travelWeight = 3.0F;
  const iggy3d::GuardDecision flipped = iggy3d::chooseSearchNode(
      g, kNoColliders, {0, 0, 0}, noMemory, 0, iggy3d::EntityId{}, homebody, kConfig);
  return expect(neutral.nodeId.has_value() && *neutral.nodeId == 1U, "neutral weights pick the far exit") &&
         expect(flipped.nodeId.has_value() && *flipped.nodeId == 0U,
                "a non-neutral travel weight flips the choice (A9 seam is live)");
}

bool unreachableNodeExcluded() {
  iggy3d::ReasoningGraph g;
  // node 0 is clear; node 1 is behind a wall AND isolated (no edges) -> unreachable -> excluded.
  g.nodes = {node(0, iggy3d::ReasoningNodeKind::exit, {5, 0, 0}),
             node(1, iggy3d::ReasoningNodeKind::exit, {0, 0, 20})};
  const std::vector<iggy3d::PhysicsAabbCollider> wall = bakeWall(-1.0F, 1.0F, 9.0F, 11.0F);
  const iggy3d::GuardMemorySample noMemory;
  const iggy3d::GuardDecision d = iggy3d::chooseSearchNode(g, wall, {0, 0, 0}, noMemory, 0,
                                                           iggy3d::EntityId{}, kNeutral, kConfig);
  return expect(!wall.empty(), "wall colliders baked") &&
         expect(d.nodeId.has_value() && *d.nodeId == 0U,
                "the walled-off, routeless node is excluded; the reachable node wins");
}

bool suppressionChoosesSecondBest() {
  iggy3d::ReasoningGraph g;
  g.nodes = {node(0, iggy3d::ReasoningNodeKind::exit, {5, 0, 0}),
             node(1, iggy3d::ReasoningNodeKind::patrolPost, {6, 0, 0})};
  const iggy3d::GuardMemorySample noMemory;
  const iggy3d::GuardDecision open = iggy3d::chooseSearchNode(g, kNoColliders, {0, 0, 0}, noMemory, 0,
                                                             iggy3d::EntityId{}, kNeutral, kConfig);
  const iggy3d::GuardDecision suppressed =
      iggy3d::chooseSearchNode(g, kNoColliders, {0, 0, 0}, noMemory, 0, iggy3d::EntityId{}, kNeutral,
                               kConfig, std::optional<std::uint32_t>{0U});
  return expect(open.nodeId.has_value() && *open.nodeId == 0U, "unsuppressed top node is the exit") &&
         expect(suppressed.nodeId.has_value() && *suppressed.nodeId == 1U,
                "suppressing the top node yields the second-best") &&
         expect(suppressed.receipt.hasExcluded && suppressed.receipt.excludedNodeId == 0U,
                "the receipt records the excluded node");
}

bool soleCandidateSuppressedYieldsNone() {
  iggy3d::ReasoningGraph g;
  g.nodes = {node(0, iggy3d::ReasoningNodeKind::exit, {5, 0, 0})};
  const iggy3d::GuardMemorySample noMemory;
  const iggy3d::GuardDecision d =
      iggy3d::chooseSearchNode(g, kNoColliders, {0, 0, 0}, noMemory, 0, iggy3d::EntityId{}, kNeutral,
                               kConfig, std::optional<std::uint32_t>{0U});
  return expect(!d.nodeId.has_value() && !d.receipt.hasChoice,
                "suppressing the sole candidate yields none-with-receipt") &&
         expect(d.receipt.hasExcluded && d.receipt.excludedNodeId == 0U,
                "the none receipt still records the exclusion");
}

bool emptyGraphYieldsNone() {
  const iggy3d::ReasoningGraph g;
  const iggy3d::GuardMemorySample noMemory;
  const iggy3d::GuardDecision d = iggy3d::chooseSearchNode(g, kNoColliders, {0, 0, 0}, noMemory, 0,
                                                           iggy3d::EntityId{}, kNeutral, kConfig);
  return expect(!d.nodeId.has_value() && !d.receipt.hasChoice, "empty graph -> none-with-receipt");
}

bool coldMemoryDecaysButStillScores() {
  iggy3d::ReasoningGraph g;
  g.nodes = {node(0, iggy3d::ReasoningNodeKind::reference, {0, 0, 0})};  // at the memory spot
  iggy3d::GuardMemorySample memory;
  memory.hasMemorySample = true;
  memory.lastKnownPosition = {0, 0, 0};
  memory.lastKnownTick = 0;

  const iggy3d::GuardDecision fresh = iggy3d::chooseSearchNode(g, kNoColliders, {5, 0, 0}, memory, 0,
                                                              iggy3d::EntityId{}, kNeutral, kConfig);
  // A cold sample: one staleness half-life later.
  const iggy3d::GuardDecision cold =
      iggy3d::chooseSearchNode(g, kNoColliders, {5, 0, 0}, memory,
                               static_cast<std::uint64_t>(kConfig.suspicionStalenessHalfLifeTicks),
                               iggy3d::EntityId{}, kNeutral, kConfig);
  iggy3d::GuardMemorySample never;  // hasMemorySample = false
  const iggy3d::GuardDecision none = iggy3d::chooseSearchNode(g, kNoColliders, {5, 0, 0}, never, 0,
                                                              iggy3d::EntityId{}, kNeutral, kConfig);

  const float freshSusp = factorValue(fresh.receipt, "suspicion");
  const float coldSusp = factorValue(cold.receipt, "suspicion");
  return expect(freshSusp > coldSusp && coldSusp > 0.0F,
                "a cold sample still scores suspicion, decayed below fresh") &&
         expect(factorValue(none.receipt, "suspicion") == 0.0F,
                "hasMemorySample=false -> suspicion 0 everywhere");
}

}  // namespace

int main() {
  const bool ok = determinismAndTieBreak() && memoryProximityDominatesNearLastKnown() &&
                  strategicSteersWhenSuspicionFlat() && travelPenalizesFarNode() &&
                  personalityWeightFlipsChoice() && unreachableNodeExcluded() &&
                  suppressionChoosesSecondBest() && soleCandidateSuppressedYieldsNone() &&
                  emptyGraphYieldsNone() && coldMemoryDecaysButStillScores();
  return ok ? 0 : 1;
}
