// a8b-s2 Commit 1 — placement + validator + report kernel, proven against fixture battlefield DATA
// (no Session). STRUCTURAL (no real-table golden; the milestone owns the one bitwise pin).

#include "runtime/encounter/EncounterPlacement.hpp"

#include "content/assets/RoomAsset.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/ai/NpcBehaviorProfile.hpp"
#include "runtime/ai/ReasoningGraph.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/encounter/EncounterDeck.hpp"
#include "runtime/physics/PhysicsSpatialSurfaceColliderBake.hpp"

#include <algorithm>
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

iggy3d::RoomAnchorAsset anchor(const std::string& kind, iggy3d::Vec3 pos) {
  iggy3d::RoomAnchorAsset a;
  a.id = "anchor_" + kind + "_" + std::to_string(static_cast<int>(pos.x)) + "_" +
         std::to_string(static_cast<int>(pos.z));
  a.kind = kind;
  a.positionMeters = pos;
  return a;
}

iggy3d::RoomSpatialSurface boxWall(const std::string& id, float x0, float x1, float z0, float z1) {
  iggy3d::RoomSpatialSurface wall;
  wall.id = id;
  wall.sourceStaticMeshId = id;
  wall.shape = iggy3d::RoomSpatialSurfaceShape::Box;
  wall.role = iggy3d::RoomSpatialSurfaceRole::Blocker;
  wall.pointsMeters = {{x0, 0.0F, z0}, {x1, 0.0F, z0}, {x1, 3.0F, z1}, {x0, 3.0F, z1}};
  wall.normal = {0.0F, 0.0F, 1.0F};
  wall.collisionMask = {"actor"};
  wall.blocksActor = true;
  return wall;
}

std::vector<iggy3d::PhysicsAabbCollider> bake(const iggy3d::RoomAsset& room) {
  const iggy3d::SpatialSurfaceSet surfaces = iggy3d::buildSpatialSurfaceSet(room);
  iggy3d::PhysicsSpatialSurfaceColliderBakeRequest request;
  request.surfaces = &surfaces;
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult result =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces(request);
  return result.ok ? result.colliders : std::vector<iggy3d::PhysicsAabbCollider>{};
}

iggy3d::ReasoningGraph graphOf(const iggy3d::RoomAsset& room) {
  std::vector<iggy3d::Vec3> waypoints;
  for (const iggy3d::RoomAnchorAsset& a : room.anchors) {
    if (a.kind == "patrol_post") {
      waypoints.push_back(a.positionMeters);
    }
  }
  return iggy3d::buildReasoningGraph(room, waypoints);
}

iggy3d::EncounterCard cardById(std::string_view id) {
  for (const iggy3d::EncounterCard& c : iggy3d::starterEncounterDeck()) {
    if (c.id == id) {
      return c;
    }
  }
  return {};
}

bool hasReason(const iggy3d::EncounterValidationResult& v, iggy3d::EncounterValidationReason r) {
  return std::find(v.reasons.begin(), v.reasons.end(), r) != v.reasons.end();
}

const iggy3d::PlacementConfig kPlacement;

bool rejectionMissingObjective() {
  // Fixture WITHOUT treasure/key/pickup + a defend_objective hand => TacticObjectiveAbsent.
  iggy3d::RoomAsset room;
  room.id = "no_objective";
  room.anchors = {anchor("spawn", {1, 0, 1}), anchor("npc", {6, 0, 6}),
                  anchor("patrol_post", {2, 0, 5}), anchor("patrol_post", {10, 0, 5})};
  const std::vector<iggy3d::PhysicsAabbCollider> colliders = bake(room);
  const iggy3d::ReasoningGraph graph = graphOf(room);
  const std::vector<iggy3d::EncounterCard> hand = {cardById("brute"), cardById("defend_objective")};

  const iggy3d::EncounterPlacementResult placement =
      iggy3d::placeEncounterHand(hand, room, graph, colliders, kPlacement);
  const iggy3d::EncounterValidationResult validation = iggy3d::validateEncounter(
      hand, placement, room, graph, colliders, iggy3d::kMediumEncounterBudget, kPlacement);
  return expect(!validation.accepted, "no-objective battlefield rejects a defend_objective hand") &&
         expect(hasReason(validation, iggy3d::EncounterValidationReason::TacticObjectiveAbsent),
                "rejection reason is TacticObjectiveAbsent");
}

bool patrolGivesRoutes() {
  iggy3d::RoomAsset room;
  room.id = "patrol_room";
  room.anchors = {anchor("spawn", {1, 0, 1}), anchor("npc", {8, 0, 1}),
                  anchor("patrol_post", {3, 0, 5}), anchor("patrol_post", {9, 0, 5})};
  const std::vector<iggy3d::PhysicsAabbCollider> colliders = bake(room);
  const std::vector<iggy3d::EncounterCard> hand = {cardById("brute"), cardById("patrol")};
  const iggy3d::EncounterPlacementResult placement =
      iggy3d::placeEncounterHand(hand, room, graphOf(room), colliders, kPlacement);
  return expect(placement.appliedTactic == "patrol", "patrol tactic applied") &&
         expect(placement.enemyAiActors.size() == 1U, "one enemy placed") &&
         expect(!placement.enemyAiActors.empty() &&
                    placement.enemyAiActors[0].patrolWaypoints.size() == 2U,
                "patrol gives the placed enemy the patrol_post waypoints (a dealt card DID change something)");
}

bool defendObjectivePicksNearest() {
  iggy3d::RoomAsset room;
  room.id = "defend_room";
  // treasure (objective) at (10,6); npc anchors near it and far from it.
  room.anchors = {anchor("spawn", {1, 0, 1}), anchor("treasure", {10, 0, 6}),
                  anchor("npc", {2, 0, 1}), anchor("npc", {9, 0, 6})};
  const std::vector<iggy3d::PhysicsAabbCollider> colliders = bake(room);
  const std::vector<iggy3d::EncounterCard> hand = {cardById("brute"), cardById("defend_objective")};
  const iggy3d::EncounterPlacementResult placement =
      iggy3d::placeEncounterHand(hand, room, graphOf(room), colliders, kPlacement);
  return expect(!placement.placements.empty() && placement.placements[0].placed,
                "brute placed under defend_objective") &&
         expect(!placement.placements.empty() &&
                    iggy3d::nearlyEqual(placement.placements[0].position, {9, 0, 6}, 0.01F),
                "defend_objective places at the npc anchor nearest the objective");
}

bool ambushWarnsWhenNoConcealment() {
  iggy3d::RoomAsset room;  // open room, no walls -> no concealed anchor
  room.id = "open_room";
  room.anchors = {anchor("spawn", {1, 0, 1}), anchor("npc", {8, 0, 1}),
                  anchor("patrol_post", {2, 0, 5}), anchor("patrol_post", {9, 0, 5})};
  const std::vector<iggy3d::PhysicsAabbCollider> colliders = bake(room);
  const std::vector<iggy3d::EncounterCard> hand = {cardById("brute"), cardById("ambush")};
  const iggy3d::EncounterPlacementResult placement =
      iggy3d::placeEncounterHand(hand, room, graphOf(room), colliders, kPlacement);
  return expect(std::find(placement.warnings.begin(), placement.warnings.end(),
                          std::string("ambush=no_concealed_slots")) != placement.warnings.end(),
                "ambush with no concealed anchor warns honestly");
}

bool clearRadiusRejects() {
  iggy3d::RoomAsset room;  // npc anchor 1 m from spawn, no wall -> LOS clear
  room.id = "too_close";
  room.anchors = {anchor("spawn", {1, 0, 1}), anchor("npc", {2, 0, 1}),
                  anchor("patrol_post", {2, 0, 5}), anchor("patrol_post", {9, 0, 5})};
  const std::vector<iggy3d::PhysicsAabbCollider> colliders = bake(room);
  const std::vector<iggy3d::EncounterCard> hand = {cardById("brute")};
  const iggy3d::EncounterPlacementResult placement =
      iggy3d::placeEncounterHand(hand, room, graphOf(room), colliders, kPlacement);
  const iggy3d::EncounterValidationResult validation = iggy3d::validateEncounter(
      hand, placement, room, graphOf(room), colliders, iggy3d::kMediumEncounterBudget, kPlacement);
  return expect(hasReason(validation, iggy3d::EncounterValidationReason::EnemyTooCloseToPlayerStart),
                "an enemy within the clear radius WITH line of sight rejects");
}

bool seedComposeIsWellFormed() {
  iggy3d::RoomAsset room;
  room.id = "compose_room";
  room.anchors = {anchor("spawn", {1, 0, 1}), anchor("exit", {12, 0, 1}),
                  anchor("treasure", {6, 0, 6}), anchor("npc", {8, 0, 1}),
                  anchor("patrol_post", {2, 0, 5}), anchor("patrol_post", {10, 0, 5})};
  const std::vector<iggy3d::PhysicsAabbCollider> colliders = bake(room);
  const std::vector<iggy3d::EncounterCard> hand = {cardById("brute"), cardById("patrol")};
  const iggy3d::EncounterPlacementResult placement =
      iggy3d::placeEncounterHand(hand, room, graphOf(room), colliders, kPlacement);
  const iggy3d::FixtureScenarioSeed seed =
      iggy3d::composeEncounterScenarioSeed(room, placement, kPlacement, iggy3d::RuntimeConfig{});

  bool ok = expect(!seed.entities.empty() && seed.entities[0].kind == iggy3d::EntityKind::Player &&
                       seed.entities[0].stableName == "player",
                   "entities[0] is the player (slot 0 => EntityId{1})");
  ok = ok && expect(seed.entities.size() == 2U, "player + one enemy entity") &&
       expect(!seed.aiActors.empty(), "aiActors present") &&
       expect(!seed.objectives.empty(), "one objective keeps the session well-formed");

  const iggy3d::NpcBehaviorProfileCatalog catalog = iggy3d::makeBuiltInNpcBehaviorProfileCatalog();
  for (const iggy3d::ScenarioAiActorSeed& aiActor : seed.aiActors) {
    bool matched = false;
    for (const iggy3d::ScenarioEntitySeed& entity : seed.entities) {
      if (entity.stableName == aiActor.actorStableName) {
        matched = true;
        break;
      }
    }
    ok = ok && expect(matched, "each aiActor pairs with an entity by stableName");
    ok = ok && expect(iggy3d::resolveNpcBehaviorProfile({&catalog, aiActor.behaviorProfileId}).ok,
                      "each aiActor's behaviorProfileId resolves");
  }
  return ok;
}

bool reportRenderIsDeterministic() {
  iggy3d::RoomAsset room;
  room.id = "report_room";
  room.anchors = {anchor("spawn", {1, 0, 1}), anchor("exit", {12, 0, 1}),
                  anchor("treasure", {6, 0, 6}), anchor("high_ground", {12, 0, 6}),
                  anchor("chokepoint", {6, 0, 2}), anchor("npc", {8, 0, 1}),
                  anchor("patrol_post", {2, 0, 5}), anchor("patrol_post", {10, 0, 5})};
  const std::vector<iggy3d::PhysicsAabbCollider> colliders = bake(room);
  const iggy3d::ReasoningGraph graph = graphOf(room);
  const iggy3d::BattlefieldFacts facts = iggy3d::battlefieldFactsFromRoom(room);
  const iggy3d::EncounterDealResult deal = iggy3d::dealEncounterHand(
      iggy3d::starterEncounterDeck(), iggy3d::kMediumEncounterBudget, 4242ULL, facts);
  const iggy3d::EncounterPlacementResult placement =
      iggy3d::placeEncounterHand(deal.hand, room, graph, colliders, kPlacement);
  const iggy3d::EncounterValidationResult validation = iggy3d::validateEncounter(
      deal.hand, placement, room, graph, colliders, iggy3d::kMediumEncounterBudget, kPlacement);
  const iggy3d::EncounterBattleReport report = iggy3d::buildEncounterBattleReport(
      room.id, deal.receipt, placement, validation, iggy3d::kMediumEncounterBudget);
  return expect(iggy3d::renderEncounterBattleReport(report) ==
                    iggy3d::renderEncounterBattleReport(report),
                "the rendered report is deterministic (call-twice self-compare)") &&
         expect(iggy3d::battlefieldFactsFromRoom(room).counts.size() >= 4U,
                "facts-from-room tallies the authored anchor kinds");
}

}  // namespace

int main() {
  const bool ok = rejectionMissingObjective() && patrolGivesRoutes() && defendObjectivePicksNearest() &&
                  ambushWarnsWhenNoConcealment() && clearRadiusRejects() && seedComposeIsWellFormed() &&
                  reportRenderIsDeterministic();
  return ok ? 0 : 1;
}
