// a8b-s2 Commit 2 — THE full-DNA milestone: one battlefield deals many battles, and proves it.
// source -> facts -> deal -> place -> validate -> report -> Session boot -> live AI, end to end.
// Carries the feature's ONE bitwise report pin (real card costs/config). The deck doc's thesis
// ("the battlefield is a possibility space, not a fixed fight") becomes a passing headless test.

#include "runtime/encounter/EncounterPlacement.hpp"

#include "content/assets/RoomAsset.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/ai/AiState.hpp"
#include "runtime/ai/NpcBehaviorProfile.hpp"
#include "runtime/ai/ReasoningGraph.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/encounter/EncounterDeck.hpp"
#include "runtime/physics/PhysicsSpatialSurfaceColliderBake.hpp"
#include "runtime/session/Session.hpp"

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

// The fixture battlefield: wire strings ONLY (fixture-graph fallback -- no Codex/ascii dependency).
// Enemy anchors sit >6 m from spawn so no enemy perceives the never-moving player, leaving the
// Patrol tactic as the sole mover (the observable liveness under player-Wait).
iggy3d::RoomAsset fixtureBattlefield() {
  iggy3d::RoomAsset room;
  room.id = "milestone";
  room.anchors = {
      anchor("spawn", {1, 0, 1}),         anchor("exit", {12, 0, 1}),
      anchor("treasure", {6, 0, 6}),      anchor("high_ground", {12, 0, 6}),
      anchor("chokepoint", {9, 0, 2}),    anchor("npc", {10, 0, 1}),
      anchor("npc", {10, 0, 6}),          anchor("patrol_post", {2, 0, 5}),
      anchor("patrol_post", {11, 0, 5}),
  };
  for (int col = 4; col <= 9; ++col) {
    for (int rw = 3; rw <= 4; ++rw) {
      room.spatialSurfaces.push_back(boxWall("wall_" + std::to_string(col) + "_" + std::to_string(rw),
                                             static_cast<float>(col) - 0.5F,
                                             static_cast<float>(col) + 0.5F,
                                             static_cast<float>(rw) - 0.5F,
                                             static_cast<float>(rw) + 0.5F));
    }
  }
  return room;
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

std::vector<std::string> keptCardIds(const iggy3d::EncounterDealReceipt& receipt) {
  std::vector<std::string> kept;
  for (const iggy3d::EncounterDealEntry& e : receipt.draws) {
    if (e.kept) {
      kept.push_back(e.cardId);
    }
  }
  return kept;
}

// The ONE tuning-coupled pin for all of A8b: the deterministic report over the REAL 9-card table at
// the discovered milestone seed (accepted medium hand including Patrol).
constexpr std::uint64_t kMilestoneSeed = 6;
constexpr const char* kExpectedReport =
    "battlefield=milestone\n"
    "seed=6\n"
    "budget=10\n"
    "cost=10\n"
    "difficulty=heavy\n"
    "tactic=patrol\n"
    "dealt=fire_barrel,trapper,patrol,rain,archer,brute,fog\n"
    "skipped=ambush:budget,defend_objective:budget\n"
    "placement.0=trapper@chokepoint#enemy_trapper_0\n"
    "placement.1=archer@high_ground#enemy_archer_0\n"
    "placement.2=brute@npc#enemy_brute_0\n"
    "validation=accepted\n";

}  // namespace

int main() {
  const iggy3d::RoomAsset room = fixtureBattlefield();
  const std::vector<iggy3d::PhysicsAabbCollider> colliders = bake(room);
  const iggy3d::ReasoningGraph graph = graphOf(room);
  const iggy3d::BattlefieldFacts facts = iggy3d::battlefieldFactsFromRoom(room);
  const iggy3d::PlacementConfig placementConfig;

  // Source -> deal -> place -> validate.
  const iggy3d::EncounterDealResult deal = iggy3d::dealEncounterHand(
      iggy3d::starterEncounterDeck(), iggy3d::kMediumEncounterBudget, kMilestoneSeed, facts);
  const iggy3d::EncounterPlacementResult placement =
      iggy3d::placeEncounterHand(deal.hand, room, graph, colliders, placementConfig);
  const iggy3d::EncounterValidationResult validation = iggy3d::validateEncounter(
      deal.hand, placement, room, graph, colliders, iggy3d::kMediumEncounterBudget, placementConfig);
  // a6s2: the report warns from the influence map computed over the placed encounter.
  const std::vector<std::string> influenceWarnings =
      iggy3d::deriveEncounterInfluenceWarnings(graph, colliders, placement, {});
  const iggy3d::EncounterBattleReport report = iggy3d::buildEncounterBattleReport(
      room.id, deal.receipt, placement, validation, iggy3d::kMediumEncounterBudget, influenceWarnings);
  const std::string rendered = iggy3d::renderEncounterBattleReport(report);
  std::cout << rendered;

  bool ok = expect(validation.accepted, "the milestone hand validates ACCEPTED");
  ok = ok && expect(rendered == kExpectedReport,
                    "the battle report matches the pinned bitwise golden (real card table)");

  // Compose -> boot the Session with the placed FixtureScenarioSeed.
  const iggy3d::FixtureScenarioSeed seed = iggy3d::composeEncounterScenarioSeed(
      room, placement, placementConfig, iggy3d::RuntimeConfig{});
  iggy3d::SessionCreateRequest request;
  request.packageId = "iggy3d.encounter_milestone";
  request.config = seed.config;
  request.seed = seed;
  iggy3d::Result<iggy3d::Session> created = iggy3d::Session::create(request);
  ok = ok && expect(created.status == iggy3d::ResultStatus::Ok, "the placed encounter boots a session");
  if (created.status != iggy3d::ResultStatus::Ok) {
    return ok ? 0 : 1;
  }
  iggy3d::Session session = std::move(created.value);
  const iggy3d::SpatialSurfaceSet surfaces = iggy3d::buildSpatialSurfaceSet(room);

  const iggy3d::EntityState* player = session.state().world.findByStableName("player");
  ok = ok && expect(player != nullptr, "player entity present at slot 0");
  const iggy3d::EntityId playerId = player != nullptr ? player->id : iggy3d::EntityId{};

  // Tick with a player Wait each tick (NoWork clock trap -- no commands => the clock never advances
  // and the AI never runs; a bare tick loop passes vacuously).
  for (int i = 0; i < 40; ++i) {
    iggy3d::CommandRecord wait;
    wait.playerSlot = 0;
    wait.actor = playerId;
    wait.kind = iggy3d::CommandKind::Wait;
    wait.source = iggy3d::CommandSource::LocalPlayer;
    (void)session.submitCommand(wait);
    ok = ok && expect(session.tick(&surfaces).status == iggy3d::ResultStatus::Ok, "encounter tick ok");
  }

  // Liveness (a): every placed enemy exists as EntityKind::Npc with a resolved profile.
  const iggy3d::NpcBehaviorProfileCatalog catalog = iggy3d::makeBuiltInNpcBehaviorProfileCatalog();
  for (const iggy3d::ScenarioAiActorSeed& aiActor : placement.enemyAiActors) {
    const iggy3d::EntityState* enemy = session.state().world.findByStableName(aiActor.actorStableName);
    ok = ok && expect(enemy != nullptr && enemy->kind == iggy3d::EntityKind::Npc,
                      "each placed enemy exists as an Npc");
    ok = ok && expect(iggy3d::resolveNpcBehaviorProfile({&catalog, aiActor.behaviorProfileId}).ok,
                      "each placed enemy's profile resolves");
  }

  // Liveness (b): a REAL decision moved off init -- at least one guard is no longer Idle/None.
  bool moved = false;
  for (const iggy3d::AiActorState& actor : session.state().ai.actors) {
    if (actor.behavior != iggy3d::AiBehaviorKind::Idle ||
        actor.lastIntent != iggy3d::AiIntentKind::None) {
      moved = true;
      break;
    }
  }
  ok = ok && expect(moved, "at least one placed guard made a real decision after N live ticks");

  // One battlefield, many battles: a different seed => a different (still valid) dealt set.
  const iggy3d::EncounterDealResult otherDeal = iggy3d::dealEncounterHand(
      iggy3d::starterEncounterDeck(), iggy3d::kMediumEncounterBudget, kMilestoneSeed + 5ULL, facts);
  ok = ok && expect(keptCardIds(otherDeal.receipt) != keptCardIds(deal.receipt),
                    "a different seed on the same battlefield deals a different battle");

  return ok ? 0 : 1;
}
