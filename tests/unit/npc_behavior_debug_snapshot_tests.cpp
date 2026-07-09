#include "runtime/ai/NpcBehaviorDebugSnapshot.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

#include "core/math/Aabb3.hpp"
#include "core/math/Transform3.hpp"
#include "runtime/combat/CombatState.hpp"
#include "runtime/world/WorldState.hpp"

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(float lhs, float rhs) {
  return std::fabs(lhs - rhs) <= 0.0001F;
}

iggy3d::Transform3 transformAt(float x, float y, float z) {
  iggy3d::Transform3 transform = iggy3d::identityTransform3();
  transform.position = {x, y, z};
  return transform;
}

iggy3d::EntityState entity(iggy3d::EntityId id,
                           std::string_view stableName,
                           iggy3d::EntityKind kind,
                           float x,
                           float z,
                           bool active = true) {
  iggy3d::EntityState result;
  result.id = id;
  result.stableName = std::string(stableName);
  result.kind = kind;
  result.transform = transformAt(x, 0.0F, z);
  result.localBounds = iggy3d::makeAabb3({-0.25F, 0.0F, -0.25F},
                                         {0.25F, 1.2F, 0.25F});
  result.active = active;
  result.persistent = true;
  return result;
}

iggy3d::AiActorState aiActor(iggy3d::EntityId actor,
                             std::string_view profileId = "default") {
  iggy3d::AiActorState state;
  state.actor = actor;
  state.behaviorProfileId = std::string(profileId);
  return state;
}

iggy3d::CombatantState combatant(iggy3d::EntityId entity,
                                 bool defeated = false) {
  return {entity, 2, defeated ? 0 : 3, 3, defeated};
}

iggy3d::NpcBehaviorDebugSnapshot snapshotFor(
    const iggy3d::WorldState& world,
    const iggy3d::AiState& ai,
    const iggy3d::CombatState& combat,
    const iggy3d::NpcBehaviorProfileCatalog& catalog,
    std::size_t maxActors = 128) {
  return iggy3d::buildNpcBehaviorDebugSnapshot({true,
                                                &world,
                                                &ai,
                                                &combat,
                                                &catalog,
                                                42,
                                                maxActors});
}

bool disabledAndMissingInputsReturnStableStatuses() {
  const iggy3d::WorldState world;
  const iggy3d::AiState ai;
  const iggy3d::CombatState combat;
  const iggy3d::NpcBehaviorProfileCatalog catalog =
      iggy3d::makeBuiltInNpcBehaviorProfileCatalog();

  const iggy3d::NpcBehaviorDebugSnapshot disabled =
      iggy3d::buildNpcBehaviorDebugSnapshot({});
  bool ok = expect(disabled.status == iggy3d::NpcBehaviorDebugSnapshotStatus::Disabled,
                   "disabled status") &&
            expect(disabled.reasonCode == "npc_behavior_debug_disabled",
                   "disabled reason") &&
            expect(disabled.actors.empty(), "disabled rows empty");

  const iggy3d::NpcBehaviorDebugSnapshot missingWorld =
      iggy3d::buildNpcBehaviorDebugSnapshot(
          {true, nullptr, &ai, &combat, &catalog, 1, 128});
  ok = ok && expect(missingWorld.status ==
                        iggy3d::NpcBehaviorDebugSnapshotStatus::MissingWorld,
                    "missing world status") &&
       expect(missingWorld.reasonCode == "npc_behavior_debug_missing_world",
              "missing world reason") &&
       expect(missingWorld.actors.empty(), "missing world rows empty");

  const iggy3d::NpcBehaviorDebugSnapshot missingAi =
      iggy3d::buildNpcBehaviorDebugSnapshot(
          {true, &world, nullptr, &combat, &catalog, 1, 128});
  ok = ok && expect(missingAi.status ==
                        iggy3d::NpcBehaviorDebugSnapshotStatus::MissingAi,
                    "missing ai status") &&
       expect(missingAi.reasonCode == "npc_behavior_debug_missing_ai",
              "missing ai reason");

  const iggy3d::NpcBehaviorDebugSnapshot missingCombat =
      iggy3d::buildNpcBehaviorDebugSnapshot(
          {true, &world, &ai, nullptr, &catalog, 1, 128});
  ok = ok && expect(missingCombat.status ==
                        iggy3d::NpcBehaviorDebugSnapshotStatus::MissingCombat,
                    "missing combat status") &&
       expect(missingCombat.reasonCode == "npc_behavior_debug_missing_combat",
              "missing combat reason");

  const iggy3d::NpcBehaviorDebugSnapshot missingCatalog =
      iggy3d::buildNpcBehaviorDebugSnapshot(
          {true, &world, &ai, &combat, nullptr, 1, 128});
  return ok && expect(missingCatalog.status ==
                          iggy3d::NpcBehaviorDebugSnapshotStatus::MissingProfileCatalog,
                      "missing profile catalog status") &&
         expect(missingCatalog.reasonCode ==
                    "npc_behavior_debug_missing_profile_catalog",
                "missing profile catalog reason");
}

bool hostileNpcRowShowsProfileTargetAndAiState() {
  iggy3d::WorldState world;
  (void)world.seedEntity(entity({1}, "training_dummy", iggy3d::EntityKind::Npc,
                                0.0F, 0.0F));
  (void)world.seedEntity(entity({2}, "player", iggy3d::EntityKind::Player,
                                3.0F, 4.0F));

  iggy3d::AiState ai;
  iggy3d::AiActorState dummy = aiActor({1});
  dummy.target = {2};
  dummy.behavior = iggy3d::AiBehaviorKind::Attacking;
  dummy.lastIntent = iggy3d::AiIntentKind::AttackTarget;
  dummy.cooldownTicksRemaining = 2;
  dummy.nextDecisionTick = 9;
  dummy.lastHorizontalAngleDeg = 12.5F;
  dummy.lastVerticalAngleDeg = -4.25F;
  dummy.lastInVerticalCone = true;
  dummy.lastPerceived = true;
  dummy.lastLos = iggy3d::AiPerceptionLos::Clear;
  dummy.lastGuardEyeHeightMeters = 1.6F;
  dummy.lastVerticalHalfAngleDegrees = 30.0F;
  ai.actors.push_back(dummy);

  iggy3d::CombatState combat;
  combat.combatants.push_back(combatant({1}));
  combat.combatants.push_back({{2}, 1, 10, 10, false});

  const iggy3d::NpcBehaviorProfileCatalog catalog =
      iggy3d::makeBuiltInNpcBehaviorProfileCatalog();
  const iggy3d::NpcBehaviorDebugSnapshot snapshot =
      snapshotFor(world, ai, combat, catalog);

  const iggy3d::NpcBehaviorDebugActorRow& row = snapshot.actors.front();
  return expect(snapshot.status == iggy3d::NpcBehaviorDebugSnapshotStatus::Ok,
                "hostile snapshot ok") &&
         expect(snapshot.reasonCode == "npc_behavior_debug_ok", "hostile reason") &&
         expect(snapshot.sourceTick == 42, "source tick") &&
         expect(snapshot.npcWorldCount == 1, "npc world count") &&
         expect(snapshot.aiActorCount == 1, "ai actor count") &&
         expect(snapshot.resolvedProfileCount == 1, "resolved profile count") &&
         expect(snapshot.failedProfileCount == 0, "failed profile count") &&
         expect(snapshot.hostileCount == 1, "hostile count") &&
         expect(snapshot.attackingCount == 1, "attacking count") &&
         expect(row.actor == iggy3d::EntityId{1}, "row actor") &&
         expect(row.stableName == "training_dummy", "stable name") &&
         expect(row.active, "row active") &&
         expect(row.isNpc, "row npc") &&
         expect(row.hasAiState, "has ai state") &&
         expect(row.hasCombatant, "has combatant") &&
         expect(!row.combatantDefeated, "combatant alive") &&
         expect(row.behaviorProfileId == "default", "profile id") &&
         expect(row.profileResolved, "profile resolved") &&
         expect(row.profileStatus == "profile_resolved", "profile status") &&
         expect(row.engagementPolicy == iggy3d::NpcEngagementPolicy::Hostile,
                "hostile policy") &&
         expect(row.behavior == iggy3d::AiBehaviorKind::Attacking,
                "behavior copied") &&
         expect(row.lastIntent == iggy3d::AiIntentKind::AttackTarget,
                "intent copied") &&
         expect(row.target == iggy3d::EntityId{2}, "target copied") &&
         expect(row.targetResolved, "target resolved") &&
         expect(row.targetStableName == "player", "target stable name") &&
         expect(row.targetActive, "target active") &&
         expect(!row.targetCombatantDefeated, "target alive") &&
         expect(near(row.targetDistanceMeters, 5.0F), "target distance") &&
         expect(near(row.lastHorizontalAngleDeg, 12.5F),
                "horizontal angle copied") &&
         expect(near(row.lastVerticalAngleDeg, -4.25F),
                "vertical angle copied") &&
         expect(row.lastInVerticalCone, "vertical cone copied") &&
         expect(row.lastPerceived, "perceived copied") &&
         expect(row.lastLos == iggy3d::AiPerceptionLos::Clear,
                "los copied") &&
         expect(near(row.lastGuardEyeHeightMeters, 1.6F),
                "guard eye copied") &&
         expect(near(row.lastVerticalHalfAngleDegrees, 30.0F),
                "vertical half angle copied") &&
         expect(row.cooldownTicksRemaining == 2, "cooldown copied") &&
         expect(row.nextDecisionTick == 9, "next decision tick");
}

bool passiveAndGhostProfilesAreCountedHonestly() {
  iggy3d::WorldState world;
  (void)world.seedEntity(entity({1}, "passive_dummy", iggy3d::EntityKind::Npc,
                                0.0F, 0.0F));
  (void)world.seedEntity(entity({2}, "ghost_dummy", iggy3d::EntityKind::Npc,
                                1.0F, 0.0F));

  iggy3d::AiState ai;
  iggy3d::AiActorState passive = aiActor({1}, "passive");
  passive.target = {2};
  passive.behavior = iggy3d::AiBehaviorKind::Alert;
  passive.lastIntent = iggy3d::AiIntentKind::Wait;
  ai.actors.push_back(passive);
  ai.actors.push_back(aiActor({2}, "ghost_profile"));

  iggy3d::CombatState combat;
  combat.combatants.push_back(combatant({1}));
  combat.combatants.push_back(combatant({2}));

  const iggy3d::NpcBehaviorProfileCatalog catalog =
      iggy3d::makeBuiltInNpcBehaviorProfileCatalog();
  const iggy3d::NpcBehaviorDebugSnapshot snapshot =
      snapshotFor(world, ai, combat, catalog);

  const iggy3d::NpcBehaviorDebugActorRow& passiveRow = snapshot.actors[0];
  const iggy3d::NpcBehaviorDebugActorRow& ghostRow = snapshot.actors[1];
  return expect(snapshot.actors.size() == 2, "two profile rows") &&
         expect(snapshot.resolvedProfileCount == 1, "one resolved") &&
         expect(snapshot.failedProfileCount == 1, "one failed") &&
         expect(snapshot.passiveCount == 1, "passive count") &&
         expect(snapshot.hostileCount == 0, "hostile count zero") &&
         expect(snapshot.waitingCount == 1, "waiting count") &&
         expect(passiveRow.profileResolved, "passive resolved") &&
         expect(passiveRow.engagementPolicy == iggy3d::NpcEngagementPolicy::Passive,
                "passive policy") &&
         expect(passiveRow.behavior == iggy3d::AiBehaviorKind::Alert,
                "passive behavior copied") &&
         expect(passiveRow.lastIntent == iggy3d::AiIntentKind::Wait,
                "passive intent copied") &&
         expect(ghostRow.behaviorProfileId == "ghost_profile", "ghost id") &&
         expect(!ghostRow.profileResolved, "ghost unresolved") &&
         expect(ghostRow.profileStatus == "profile_missing", "ghost status");
}

bool missingAiAndBrokenActorRowsAreVisible() {
  iggy3d::WorldState world;
  (void)world.seedEntity(entity({1}, "auto_npc", iggy3d::EntityKind::Npc,
                                0.0F, 0.0F));
  (void)world.seedEntity(entity({2}, "door", iggy3d::EntityKind::Door, 1.0F, 0.0F));

  iggy3d::AiState ai;
  ai.actors.push_back(aiActor({2}, "passive"));
  ai.actors.push_back(aiActor({99}, "default"));

  iggy3d::CombatState combat;
  combat.combatants.push_back(combatant({1}));

  const iggy3d::NpcBehaviorProfileCatalog catalog =
      iggy3d::makeBuiltInNpcBehaviorProfileCatalog();
  const iggy3d::NpcBehaviorDebugSnapshot snapshot =
      snapshotFor(world, ai, combat, catalog);

  return expect(snapshot.actors.size() == 3, "three diagnostic rows") &&
         expect(snapshot.actors[0].actor == iggy3d::EntityId{1}, "row one sorted") &&
         expect(snapshot.actors[0].isNpc, "npc row is npc") &&
         expect(!snapshot.actors[0].hasAiState, "npc without ai visible") &&
         expect(snapshot.actors[0].behaviorProfileId == "default",
                "missing ai default profile") &&
         expect(snapshot.actors[0].profileResolved, "missing ai default resolved") &&
         expect(snapshot.actors[1].actor == iggy3d::EntityId{2}, "row two sorted") &&
         expect(snapshot.actors[1].stableName == "door", "non npc stable name") &&
         expect(!snapshot.actors[1].isNpc, "non npc diagnostic") &&
         expect(snapshot.actors[1].hasAiState, "non npc has ai state") &&
         expect(snapshot.actors[2].actor == iggy3d::EntityId{99},
                "missing world actor sorted") &&
         expect(snapshot.actors[2].stableName == "none", "missing world stable") &&
         expect(!snapshot.actors[2].active, "missing world inactive") &&
         expect(!snapshot.actors[2].isNpc, "missing world not npc") &&
         expect(snapshot.actors[2].hasAiState, "missing world has ai state");
}

bool maxActorCapIsDeterministic() {
  iggy3d::WorldState world;
  (void)world.seedEntity(entity({1}, "npc_1", iggy3d::EntityKind::Npc, 0.0F, 0.0F));
  (void)world.seedEntity(entity({2}, "npc_2", iggy3d::EntityKind::Npc, 1.0F, 0.0F));
  (void)world.seedEntity(entity({3}, "npc_3", iggy3d::EntityKind::Npc, 2.0F, 0.0F));

  iggy3d::AiState ai;
  ai.actors.push_back(aiActor({1}));
  ai.actors.push_back(aiActor({2}));
  ai.actors.push_back(aiActor({3}));

  iggy3d::CombatState combat;
  const iggy3d::NpcBehaviorProfileCatalog catalog =
      iggy3d::makeBuiltInNpcBehaviorProfileCatalog();
  const iggy3d::NpcBehaviorDebugSnapshot snapshot =
      snapshotFor(world, ai, combat, catalog, 2);

  return expect(snapshot.npcWorldCount == 3, "cap keeps world count") &&
         expect(snapshot.aiActorCount == 3, "cap keeps ai count") &&
         expect(snapshot.actors.size() == 2, "cap rows") &&
         expect(snapshot.actors[0].actor == iggy3d::EntityId{1}, "cap first") &&
         expect(snapshot.actors[1].actor == iggy3d::EntityId{2}, "cap second");
}

}  // namespace

int main() {
  bool ok = true;
  ok = ok && disabledAndMissingInputsReturnStableStatuses();
  ok = ok && hostileNpcRowShowsProfileTargetAndAiState();
  ok = ok && passiveAndGhostProfilesAreCountedHonestly();
  ok = ok && missingAiAndBrokenActorRowsAreVisible();
  ok = ok && maxActorCapIsDeterministic();
  if (!ok) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
