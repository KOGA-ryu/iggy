#include "runtime/session/Session.hpp"
#include "runtime/session/SessionTick.hpp"

#include "runtime/ai/NpcAlertSystem.hpp"
#include "runtime/ai/NpcInvestigateSystem.hpp"
#include "runtime/ai/NpcPatrolSystem.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/inventory/InventorySystem.hpp"
#include "runtime/objective/ObjectiveSystem.hpp"

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

iggy3d::Transform3 transformAt(float x, float y, float z) {
  iggy3d::Transform3 transform = iggy3d::identityTransform3();
  transform.position = {x, y, z};
  return transform;
}

iggy3d::ScenarioEntitySeed playerSeed() {
  iggy3d::ScenarioEntitySeed seed;
  seed.stableName = "player";
  seed.kind = iggy3d::EntityKind::Player;
  seed.transform = transformAt(0.0F, 0.0F, 0.0F);
  seed.localBounds = iggy3d::makeAabb3({-0.25F, 0.0F, -0.25F}, {0.25F, 1.8F, 0.25F});
  seed.active = true;
  seed.persistent = true;
  return seed;
}

iggy3d::ScenarioEntitySeed combatPlayerSeed() {
  iggy3d::ScenarioEntitySeed seed = playerSeed();
  seed.targeting.targetable = true;
  seed.targeting.actions = {iggy3d::TargetAction::Attack, iggy3d::TargetAction::Inspect};
  seed.combatantEnabled = true;
  seed.combatant.factionId = 1;
  seed.combatant.hitPoints = 10;
  seed.combatant.maxHitPoints = 10;
  return seed;
}

iggy3d::ScenarioEntitySeed nonAttackTargetableCombatPlayerSeed() {
  iggy3d::ScenarioEntitySeed seed = combatPlayerSeed();
  seed.targeting.targetable = false;
  seed.targeting.actions.clear();
  return seed;
}

iggy3d::ScenarioEntitySeed goldKeySeed() {
  iggy3d::ScenarioEntitySeed seed;
  seed.stableName = "gold_key";
  seed.kind = iggy3d::EntityKind::Pickup;
  seed.transform = transformAt(3.0F, 0.0F, 0.0F);
  seed.localBounds = iggy3d::makeAabb3({-0.1F, 0.0F, -0.1F}, {0.1F, 0.1F, 0.1F});
  seed.active = true;
  seed.persistent = true;
  seed.targeting.targetable = true;
  seed.targeting.actions = {iggy3d::TargetAction::Interact, iggy3d::TargetAction::Inspect};
  seed.interaction.kind = iggy3d::InteractionKind::Pickup;
  seed.interaction.primaryEffect = iggy3d::InteractionEffectKind::AddItemToInventory;
  seed.interaction.itemId = "gold_key";
  seed.interaction.itemCount = 1;
  seed.interaction.objectiveId = "collect_gold_key";
  seed.interaction.deactivateTargetOnSuccess = true;
  return seed;
}

iggy3d::ScenarioEntitySeed markerSeed() {
  iggy3d::ScenarioEntitySeed seed;
  seed.stableName = "tactical_marker_alpha";
  seed.kind = iggy3d::EntityKind::Marker;
  seed.transform = transformAt(2.0F, 0.0F, 1.0F);
  seed.localBounds = iggy3d::makeAabb3({-0.1F, 0.0F, -0.1F}, {0.1F, 0.1F, 0.1F});
  seed.active = true;
  seed.persistent = true;
  seed.targeting.targetable = true;
  seed.targeting.actions = {iggy3d::TargetAction::Move, iggy3d::TargetAction::Inspect};
  return seed;
}

iggy3d::ScenarioEntitySeed trainingNpcSeed() {
  iggy3d::ScenarioEntitySeed seed;
  seed.stableName = "training_npc";
  seed.kind = iggy3d::EntityKind::Npc;
  seed.transform = transformAt(1.0F, 0.0F, 0.0F);
  seed.localBounds = iggy3d::makeAabb3({-0.25F, 0.0F, -0.25F}, {0.25F, 1.2F, 0.25F});
  seed.active = true;
  seed.persistent = true;
  seed.targeting.targetable = true;
  seed.targeting.actions = {iggy3d::TargetAction::Attack, iggy3d::TargetAction::Inspect};
  seed.combatantEnabled = true;
  seed.combatant.factionId = 2;
  seed.combatant.hitPoints = 3;
  seed.combatant.maxHitPoints = 3;
  return seed;
}

iggy3d::ScenarioEntitySeed trainingNpcSeedAt(float x) {
  iggy3d::ScenarioEntitySeed seed = trainingNpcSeed();
  seed.transform = transformAt(x, 0.0F, 0.0F);
  return seed;
}

iggy3d::ScenarioEntitySeed loopKeySeed() {
  iggy3d::ScenarioEntitySeed seed;
  seed.stableName = "marker_key_r1_c1";
  seed.kind = iggy3d::EntityKind::Pickup;
  seed.transform = transformAt(1.0F, 0.0F, 0.0F);
  seed.localBounds = iggy3d::makeAabb3({-0.1F, 0.0F, -0.1F}, {0.1F, 0.1F, 0.1F});
  seed.active = true;
  seed.persistent = true;
  seed.targeting.targetable = true;
  seed.targeting.actions = {iggy3d::TargetAction::Interact, iggy3d::TargetAction::Inspect};
  seed.interaction.kind = iggy3d::InteractionKind::Pickup;
  seed.interaction.primaryEffect = iggy3d::InteractionEffectKind::AddItemToInventory;
  seed.interaction.itemId = "marker_key_r1_c1";
  seed.interaction.itemCount = 1;
  seed.interaction.objectiveId = "collect_marker_key_r1_c1";
  seed.interaction.deactivateTargetOnSuccess = true;
  return seed;
}

iggy3d::ScenarioEntitySeed loopSecretDoorSeed() {
  iggy3d::ScenarioEntitySeed seed;
  seed.stableName = "marker_secret_door_r1_c2";
  seed.kind = iggy3d::EntityKind::Door;
  seed.transform = transformAt(2.0F, 0.0F, 0.0F);
  seed.localBounds = iggy3d::makeAabb3({-0.25F, 0.0F, -0.25F}, {0.25F, 1.8F, 0.25F});
  seed.active = true;
  seed.persistent = true;
  seed.targeting.targetable = true;
  seed.targeting.actions = {iggy3d::TargetAction::Interact, iggy3d::TargetAction::Inspect};
  seed.interaction.kind = iggy3d::InteractionKind::OpenDoor;
  seed.interaction.primaryEffect = iggy3d::InteractionEffectKind::EmitEventOnly;
  seed.interaction.requiredItemId = "marker_key_r1_c1";
  seed.interaction.requiredItemCount = 1;
  seed.interaction.deactivateTargetOnSuccess = true;
  return seed;
}

iggy3d::ScenarioEntitySeed loopTreasureSeed() {
  iggy3d::ScenarioEntitySeed seed;
  seed.stableName = "marker_treasure_r1_c3";
  seed.kind = iggy3d::EntityKind::Pickup;
  seed.transform = transformAt(3.0F, 0.0F, 0.0F);
  seed.localBounds = iggy3d::makeAabb3({-0.1F, 0.0F, -0.1F}, {0.1F, 0.1F, 0.1F});
  seed.active = true;
  seed.persistent = true;
  seed.targeting.targetable = true;
  seed.targeting.actions = {iggy3d::TargetAction::Interact, iggy3d::TargetAction::Inspect};
  seed.interaction.kind = iggy3d::InteractionKind::Pickup;
  seed.interaction.primaryEffect = iggy3d::InteractionEffectKind::AddItemToInventory;
  seed.interaction.itemId = "marker_treasure_r1_c3";
  seed.interaction.itemCount = 1;
  seed.interaction.objectiveId = "collect_marker_treasure_r1_c3";
  seed.interaction.deactivateTargetOnSuccess = true;
  return seed;
}

iggy3d::ScenarioEntitySeed loopExitSeed() {
  iggy3d::ScenarioEntitySeed seed;
  seed.stableName = "marker_exit_r1_c4";
  seed.kind = iggy3d::EntityKind::Marker;
  seed.transform = transformAt(4.0F, 0.0F, 0.0F);
  seed.localBounds = iggy3d::makeAabb3({-0.1F, 0.0F, -0.1F}, {0.1F, 0.1F, 0.1F});
  seed.active = true;
  seed.persistent = true;
  seed.targeting.targetable = true;
  seed.targeting.actions = {iggy3d::TargetAction::Interact, iggy3d::TargetAction::Move,
                            iggy3d::TargetAction::Inspect};
  seed.interaction.kind = iggy3d::InteractionKind::ObjectiveTrigger;
  seed.interaction.primaryEffect = iggy3d::InteractionEffectKind::CompleteObjective;
  seed.interaction.objectiveId = "exit_marker_exit_r1_c4";
  seed.interaction.requiredItemId = "marker_treasure_r1_c3";
  seed.interaction.requiredItemCount = 1;
  return seed;
}

iggy3d::ScenarioObjectiveSeed collectObjective(std::string_view id,
                                               std::string_view itemId) {
  iggy3d::ScenarioObjectiveSeed objective;
  objective.id = std::string(id);
  objective.initialStatus = iggy3d::ObjectiveStatusSeed::Active;
  objective.condition = "InventoryContains";
  objective.playerSlot = 0;
  objective.itemId = std::string(itemId);
  objective.itemCount = 1;
  objective.completeStatus = iggy3d::ObjectiveStatusSeed::Complete;
  return objective;
}

iggy3d::ScenarioObjectiveSeed exitObjective() {
  iggy3d::ScenarioObjectiveSeed objective;
  objective.id = "exit_marker_exit_r1_c4";
  objective.initialStatus = iggy3d::ObjectiveStatusSeed::Active;
  objective.condition = "None";
  objective.playerSlot = 0;
  objective.completeStatus = iggy3d::ObjectiveStatusSeed::Complete;
  return objective;
}

iggy3d::FixtureScenarioSeed makeFirstRoomSeed() {
  iggy3d::FixtureScenarioSeed seed;
  seed.scenarioId = "first_room.runtime_loop";
  seed.config = iggy3d::makeDefaultRuntimeConfig();
  seed.initialClockMode = iggy3d::ClockMode::Normal;
  seed.defaultRealtimeCamera = iggy3d::CameraMode::ThirdPerson;
  seed.defaultTacticalCamera = iggy3d::CameraMode::TacticalOverhead;
  seed.players.push_back({0, iggy3d::PlayerSlotKind::Local, "player"});
  seed.entities = {playerSeed(), goldKeySeed(), markerSeed()};
  iggy3d::ScenarioObjectiveSeed objective;
  objective.id = "collect_gold_key";
  objective.initialStatus = iggy3d::ObjectiveStatusSeed::Active;
  objective.condition = "InventoryContains";
  objective.playerSlot = 0;
  objective.itemId = "gold_key";
  objective.itemCount = 1;
  objective.completeStatus = iggy3d::ObjectiveStatusSeed::Complete;
  seed.objectives.push_back(objective);
  return seed;
}

iggy3d::FixtureScenarioSeed makeExitLoopSeed() {
  iggy3d::FixtureScenarioSeed seed;
  seed.scenarioId = "ascii_exit_loop.runtime_loop";
  seed.config = iggy3d::makeDefaultRuntimeConfig();
  seed.initialClockMode = iggy3d::ClockMode::Normal;
  seed.defaultRealtimeCamera = iggy3d::CameraMode::FirstPerson;
  seed.defaultTacticalCamera = iggy3d::CameraMode::TacticalOverhead;
  seed.players.push_back({0, iggy3d::PlayerSlotKind::Local, "player"});
  seed.entities = {playerSeed(), loopKeySeed(), loopSecretDoorSeed(), loopTreasureSeed(),
                   loopExitSeed()};
  seed.objectives = {
      collectObjective("collect_marker_key_r1_c1", "marker_key_r1_c1"),
      collectObjective("collect_marker_treasure_r1_c3", "marker_treasure_r1_c3"),
      exitObjective(),
  };
  return seed;
}

iggy3d::FixtureScenarioSeed makeNpcCombatSeed(float npcX = 1.0F,
                                              bool playerAttackTargetable = true) {
  iggy3d::FixtureScenarioSeed seed;
  seed.scenarioId = "npc_behavior.runtime_loop";
  seed.config = iggy3d::makeDefaultRuntimeConfig();
  seed.initialClockMode = iggy3d::ClockMode::Normal;
  seed.defaultRealtimeCamera = iggy3d::CameraMode::ThirdPerson;
  seed.defaultTacticalCamera = iggy3d::CameraMode::TacticalOverhead;
  seed.players.push_back({0, iggy3d::PlayerSlotKind::Local, "player"});
  seed.entities = {playerAttackTargetable ? combatPlayerSeed()
                                          : nonAttackTargetableCombatPlayerSeed(),
                   trainingNpcSeedAt(npcX)};
  seed.objectives.push_back(collectObjective("collect_unreachable", "unreachable"));
  return seed;
}

iggy3d::FixtureScenarioSeed makeNpcGuardSeed(float npcX = 1.0F,
                                             bool playerAttackTargetable = true) {
  iggy3d::FixtureScenarioSeed seed =
      makeNpcCombatSeed(npcX, playerAttackTargetable);
  seed.entities.push_back(markerSeed());
  return seed;
}

iggy3d::Session makeSession() {
  iggy3d::SessionCreateRequest request;
  request.config = iggy3d::makeDefaultRuntimeConfig();
  request.seed = makeFirstRoomSeed();
  return iggy3d::Session::create(request).value;
}

iggy3d::Session makeExitLoopSession() {
  iggy3d::SessionCreateRequest request;
  request.config = iggy3d::makeDefaultRuntimeConfig();
  request.seed = makeExitLoopSeed();
  return iggy3d::Session::create(request).value;
}

iggy3d::Session makeNpcCombatSession(float npcX = 1.0F,
                                     bool playerAttackTargetable = true) {
  iggy3d::SessionCreateRequest request;
  request.config = iggy3d::makeDefaultRuntimeConfig();
  request.seed = makeNpcCombatSeed(npcX, playerAttackTargetable);
  return iggy3d::Session::create(request).value;
}

iggy3d::Result<iggy3d::Session> createNpcCombatSessionWithAiSeed(
    std::string_view actorStableName,
    std::string_view behaviorProfileId,
    float npcX = 1.0F) {
  iggy3d::SessionCreateRequest request;
  request.config = iggy3d::makeDefaultRuntimeConfig();
  request.seed = makeNpcCombatSeed(npcX);
  request.seed.aiActors.push_back(
      {std::string(actorStableName), std::string(behaviorProfileId)});
  return iggy3d::Session::create(request);
}

iggy3d::CommandRecord submittedInteract() {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Interact;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.target.hasEntity = true;
  command.payload.target.entity = {2};
  return command;
}

iggy3d::CommandRecord submittedInteractTarget(iggy3d::EntityId target) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Interact;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.target.hasEntity = true;
  command.payload.target.entity = target;
  return command;
}

iggy3d::CommandRecord submittedMove(iggy3d::Vec3 point) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Move;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.target.hasPoint = true;
  command.payload.target.point = point;
  return command;
}

iggy3d::CommandRecord submittedWait() {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Wait;
  command.source = iggy3d::CommandSource::LocalPlayer;
  return command;
}

iggy3d::CommandRecord submittedRetry(iggy3d::CommandId sourceCommandId) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Retry;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.retrySourceCommandId = sourceCommandId;
  return command;
}

iggy3d::RoomSpatialSurface tickFloorSurface() {
  iggy3d::RoomSpatialSurface surface;
  surface.id = "tick_floor";
  surface.sourceStaticMeshId = "tick_floor";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Plane;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Walkable;
  surface.pointsMeters = {
      {-10.0F, 0.0F, -10.0F},
      {10.0F, 0.0F, -10.0F},
      {10.0F, 0.0F, 10.0F},
      {-10.0F, 0.0F, 10.0F},
  };
  surface.normal = {0.0F, 1.0F, 0.0F};
  surface.collisionMask = {"actor"};
  return surface;
}

iggy3d::RoomSpatialSurface tickWallSurface() {
  iggy3d::RoomSpatialSurface surface;
  surface.id = "tick_wall";
  surface.sourceStaticMeshId = "tick_wall";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Box;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Blocker;
  surface.pointsMeters = {
      {-10.0F, 0.0F, -1.10F},
      {10.0F, 0.0F, -1.10F},
      {10.0F, 3.0F, -0.90F},
      {-10.0F, 3.0F, -0.90F},
  };
  surface.normal = {0.0F, 0.0F, 1.0F};
  surface.collisionMask = {"actor"};
  surface.blocksActor = true;
  return surface;
}

iggy3d::SpatialSurfaceSet tickCollisionSurfaces() {
  iggy3d::RoomAsset room;
  room.id = "tick_collision_room";
  room.spatialSurfaces = {tickFloorSurface(), tickWallSurface()};
  return iggy3d::buildSpatialSurfaceSet(room);
}

std::uint32_t itemCount(const iggy3d::InventoryState& inventory, std::string_view itemId) {
  const iggy3d::PlayerInventory* player = iggy3d::findInventory(inventory, 0);
  if (player == nullptr) {
    return 0;
  }
  for (const iggy3d::InventoryStack& stack : player->stacks) {
    if (stack.itemId == itemId) {
      return stack.count;
    }
  }
  return 0;
}

std::uint32_t goldKeyCount(const iggy3d::InventoryState& inventory) {
  return itemCount(inventory, "gold_key");
}

const iggy3d::CombatantState* findCombatant(const iggy3d::CombatState& combat,
                                            iggy3d::EntityId entity) {
  for (const iggy3d::CombatantState& combatant : combat.combatants) {
    if (combatant.entity == entity) {
      return &combatant;
    }
  }
  return nullptr;
}

iggy3d::CombatantState* findMutableCombatant(iggy3d::CombatState& combat,
                                             iggy3d::EntityId entity) {
  for (iggy3d::CombatantState& combatant : combat.combatants) {
    if (combatant.entity == entity) {
      return &combatant;
    }
  }
  return nullptr;
}

const iggy3d::AiActorState* findAiActor(const iggy3d::AiState& ai,
                                        iggy3d::EntityId actor) {
  for (const iggy3d::AiActorState& actorState : ai.actors) {
    if (actorState.actor == actor) {
      return &actorState;
    }
  }
  return nullptr;
}

void seedNpcAiProfile(iggy3d::Session& session, std::string_view profileId) {
  iggy3d::AiActorState actor;
  actor.actor = {2};
  actor.behaviorProfileId = std::string(profileId);
  // These fixtures place the player at the origin with the NPC on the +x axis,
  // so face -x toward the player to bring it inside the NPC's vision cone.
  actor.facingDirection = {-1.0F, 0.0F, 0.0F};
  auto& actors = session.mutableStateForOwnedSystems().ai.actors;
  actors.clear();
  actors.push_back(actor);
}

// Seed the NPC AI actor and pre-force its graded alert to the combat band (1.0)
// so the plumbing tests below keep the instant chase/attack path. Escalation
// itself is proven separately by npcAlertLadderEscalatesThenDecaysInLoop, so
// these tests stay tuning-agnostic while still honoring the new alert gate.
void seedAlertedNpcAtCombat(iggy3d::Session& session, std::string_view profileId) {
  seedNpcAiProfile(session, profileId);  // actor {2}, facing -x toward player
  for (auto& a : session.mutableStateForOwnedSystems().ai.actors) {
    if (a.actor == iggy3d::EntityId{2}) {
      a.alertLevel = 1.0F;
      a.lastRiseTick = 0;
    }
  }
}

// Seed the NPC actor {2} with an authored patrol route, facing +z so the player at the
// origin sits OUTSIDE its vision cone (the NPC stays at low alert and patrols).
void seedPatrolNpc(iggy3d::Session& session,
                   const std::vector<iggy3d::Vec3>& waypoints,
                   iggy3d::PatrolMode mode) {
  iggy3d::AiActorState actor;
  actor.actor = {2};
  actor.behaviorProfileId = "default";
  actor.facingDirection = {0.0F, 0.0F, 1.0F};  // +z: player (at -x) is out of cone
  actor.patrolWaypoints = waypoints;
  actor.patrolMode = mode;
  auto& actors = session.mutableStateForOwnedSystems().ai.actors;
  actors.clear();
  actors.push_back(actor);
}

iggy3d::AiActorState* mutableAiActor(iggy3d::Session& session, iggy3d::EntityId actor) {
  for (iggy3d::AiActorState& a : session.mutableStateForOwnedSystems().ai.actors) {
    if (a.actor == actor) {
      return &a;
    }
  }
  return nullptr;
}

const iggy3d::CommandRecord* lastCommandWithSource(const iggy3d::CommandLog& log,
                                                   iggy3d::CommandSource source) {
  const iggy3d::CommandRecord* result = nullptr;
  for (const iggy3d::CommandRecord& record : log.records()) {
    if (record.source == source) {
      result = &record;
    }
  }
  return result;
}

bool tickMovesThenRetryPicksUpKeyExactlyOnce() {
  iggy3d::Session session = makeSession();
  const iggy3d::SessionCommandResult rejectedInteract = session.submitCommand(submittedInteract());
  bool ok = expect(rejectedInteract.command.commandId == 1U, "rejected interact id") &&
            expect(rejectedInteract.command.rejection == iggy3d::CommandRejectionReason::OutOfRange,
                   "rejected out of range") &&
            expect(session.state().transient.pendingExecutionSequences.empty(),
                   "rejected not queued");

  const iggy3d::SessionCommandResult move =
      session.submitCommand(submittedMove({2.0F, 0.0F, 0.0F}));
  ok = ok && expect(move.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
                    "move accepted") &&
       expect(session.state().transient.pendingExecutionSequences.size() == 1U, "move queued");

  ok = ok && expect(session.tick().status == iggy3d::ResultStatus::Ok, "move tick ok") &&
       expect(session.state().transient.pendingExecutionSequences.empty(), "move queue consumed") &&
       expect(iggy3d::nearlyEqual(session.state().world.findById({1})->transform.position,
                                  {2.0F, 0.0F, 0.0F}),
              "player moved") &&
       expect(session.state().clock.tickIndex == 1U, "tick advanced after move");

  const iggy3d::SessionCommandResult retry = session.submitCommand(submittedRetry(1));
  ok = ok && expect(retry.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
                    "retry accepted") &&
       expect(retry.command.kind == iggy3d::CommandKind::Retry, "retry logged as retry") &&
       expect(session.state().transient.pendingExecutionSequences.size() == 1U, "retry queued");

  ok = ok && expect(session.tick().status == iggy3d::ResultStatus::Ok, "retry tick ok") &&
       expect(session.state().transient.pendingExecutionSequences.empty(), "retry queue consumed") &&
       expect(goldKeyCount(session.state().inventory) == 1U, "one key acquired") &&
       expect(!session.state().world.findById({2})->active, "key inactive") &&
       expect(iggy3d::objectiveComplete(session.state().objectives, "collect_gold_key"),
              "objective complete") &&
       expect(session.state().outcome == iggy3d::SessionOutcome::DemoComplete,
              "demo complete outcome") &&
       expect(session.state().lifecycle == iggy3d::SessionLifecycle::Playing,
              "lifecycle remains playing");

  ok = ok && expect(session.tick().status == iggy3d::ResultStatus::Ok, "idle tick ok") &&
       expect(goldKeyCount(session.state().inventory) == 1U, "idle no duplicate key") &&
       expect(session.state().transient.pendingExecutionSequences.empty(), "idle queue empty");
  return ok;
}

bool exitObjectiveCompletionSetsVictoryAfterRequiredItems() {
  iggy3d::Session session = makeExitLoopSession();

  const iggy3d::SessionCommandResult pickupKey =
      session.submitCommand(submittedInteractTarget({2}));
  bool ok = expect(pickupKey.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
                   "key pickup accepted") &&
            expect(session.tick().status == iggy3d::ResultStatus::Ok, "key tick ok") &&
            expect(itemCount(session.state().inventory, "marker_key_r1_c1") == 1U,
                   "key acquired") &&
            expect(iggy3d::objectiveComplete(session.state().objectives,
                                             "collect_marker_key_r1_c1"),
                   "key objective complete") &&
            expect(session.state().outcome == iggy3d::SessionOutcome::None,
                   "key does not finish loop");

  const iggy3d::SessionCommandResult moveToDoor =
      session.submitCommand(submittedMove({1.0F, 0.0F, 0.0F}));
  ok = ok && expect(moveToDoor.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
                    "move to door accepted") &&
       expect(session.tick().status == iggy3d::ResultStatus::Ok, "move to door tick ok");

  const iggy3d::SessionCommandResult openSecretDoor =
      session.submitCommand(submittedInteractTarget({3}));
  ok = ok && expect(openSecretDoor.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
                    "secret door accepted with key") &&
       expect(session.tick().status == iggy3d::ResultStatus::Ok, "secret door tick ok") &&
       expect(!session.state().world.findById({3})->active, "secret door inactive");

  const iggy3d::SessionCommandResult moveToTreasure =
      session.submitCommand(submittedMove({3.0F, 0.0F, 0.0F}));
  ok = ok && expect(moveToTreasure.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
                    "move to treasure accepted") &&
       expect(session.tick().status == iggy3d::ResultStatus::Ok, "move to treasure tick ok");

  const iggy3d::SessionCommandResult rejectedExit =
      session.submitCommand(submittedInteractTarget({5}));
  ok = ok &&
       expect(rejectedExit.command.admission == iggy3d::CommandAdmissionStatus::Rejected,
              "exit rejected before treasure") &&
       expect(rejectedExit.command.rejection == iggy3d::CommandRejectionReason::RequiredItemMissing,
              "exit missing treasure reason") &&
       expect(!iggy3d::objectiveComplete(session.state().objectives,
                                         "exit_marker_exit_r1_c4"),
              "exit objective still active") &&
       expect(session.state().outcome == iggy3d::SessionOutcome::None,
              "rejected exit no outcome");

  const iggy3d::SessionCommandResult pickupTreasure =
      session.submitCommand(submittedInteractTarget({4}));
  ok = ok && expect(pickupTreasure.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
                    "treasure pickup accepted") &&
       expect(session.tick().status == iggy3d::ResultStatus::Ok, "treasure tick ok") &&
       expect(itemCount(session.state().inventory, "marker_treasure_r1_c3") == 1U,
              "treasure acquired") &&
       expect(iggy3d::objectiveComplete(session.state().objectives,
                                        "collect_marker_treasure_r1_c3"),
              "treasure objective complete") &&
       expect(session.state().outcome == iggy3d::SessionOutcome::None,
              "treasure alone does not finish loop");

  const iggy3d::SessionCommandResult finishExit =
      session.submitCommand(submittedInteractTarget({5}));
  ok = ok && expect(finishExit.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
                    "exit accepted after treasure") &&
       expect(session.tick().status == iggy3d::ResultStatus::Ok, "exit tick ok") &&
       expect(iggy3d::objectiveComplete(session.state().objectives,
                                        "exit_marker_exit_r1_c4"),
              "exit objective complete") &&
       expect(session.state().outcome == iggy3d::SessionOutcome::Victory,
              "exit objective sets victory") &&
       expect(session.state().lifecycle == iggy3d::SessionLifecycle::Playing,
              "victory keeps lifecycle playing");

  const iggy3d::SessionCommandResult wait = session.submitCommand(submittedWait());
  ok = ok && expect(wait.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
                    "post victory wait accepted") &&
       expect(session.tick().status == iggy3d::ResultStatus::Ok, "post victory wait tick ok") &&
       expect(session.state().outcome == iggy3d::SessionOutcome::Victory,
              "victory not downgraded");
  return ok;
}

bool collisionBlockedMoveConsumesPendingCommandWithoutMutation() {
  iggy3d::Session session = makeSession();
  const iggy3d::SpatialSurfaceSet surfaces = tickCollisionSurfaces();

  const iggy3d::SessionCommandResult move =
      session.submitCommand(submittedMove({0.0F, 0.0F, -2.0F}));
  bool ok = expect(move.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
                   "collision move accepted") &&
            expect(session.state().transient.pendingExecutionSequences.size() == 1U,
                   "collision move queued");

  ok = ok && expect(session.tick(&surfaces).status == iggy3d::ResultStatus::Ok,
                    "collision blocked tick ok") &&
       expect(session.state().transient.pendingExecutionSequences.empty(),
              "collision blocked queue consumed") &&
       expect(iggy3d::nearlyEqual(session.state().world.findById({1})->transform.position,
                                  {0.0F, 0.0F, 0.0F}),
              "collision blocked no mutation") &&
       expect(session.state().transient.lastMovementResultAvailable,
              "collision movement result available") &&
       expect(session.state().transient.lastMovementResult.blocked ==
                  iggy3d::MovementBlockedReason::BlockedByCollision,
              "collision movement blocked reason") &&
       expect(!session.state().transient.lastMovementResult.physicsFrameStatsAvailable,
              "collision movement no physics stats") &&
       expect(session.state().transient.lastMovementResult.hitSurfaceId == "tick_wall",
              "collision movement hit surface id") &&
       expect(session.state().transient.lastMovementResult.movementClamped,
              "collision movement clamp flag") &&
       expect(session.state().clock.tickIndex == 1U,
              "collision blocked tick advanced");

  const iggy3d::SessionCommandResult openMove =
      session.submitCommand(submittedMove({1.0F, 0.0F, 0.0F}));
  ok = ok && expect(openMove.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
                    "open move accepted after collision block") &&
       expect(session.tick(&surfaces).status == iggy3d::ResultStatus::Ok,
              "open move tick ok after collision block") &&
       expect(iggy3d::nearlyEqual(session.state().world.findById({1})->transform.position,
                                  {1.0F, 0.0F, 0.0F}),
              "open move mutates after collision block") &&
       expect(session.state().transient.lastMovementResultAvailable,
              "open movement result available") &&
       expect(session.state().transient.lastMovementResult.blocked ==
                  iggy3d::MovementBlockedReason::None,
              "open movement ok reason") &&
       expect(session.state().transient.lastMovementResult.hitSurfaceId.empty(),
              "open movement no hit surface") &&
       expect(session.state().transient.lastMovementResult.movementPolicyBand == "flat",
              "open movement flat slope band");
  return ok;
}

bool sessionTickInputPhysicsPlannerDefaultsOff() {
  iggy3d::Session session = makeSession();
  const iggy3d::SpatialSurfaceSet surfaces = tickCollisionSurfaces();
  const iggy3d::SessionCommandResult move =
      session.submitCommand(submittedMove({0.0F, 0.0F, -2.0F}));
  std::vector<iggy3d::CommandRecord> commands = {move.command};

  iggy3d::SessionTickInput input;
  input.state = &session.mutableStateForOwnedSystems();
  input.acceptedCommands = commands;
  input.collisionSurfaces = &surfaces;

  const iggy3d::SessionTickResult tick = iggy3d::runSessionTick(input);
  return expect(!input.usePhysicsMovePlanner, "tick input physics default false") &&
         expect(tick.status == iggy3d::SessionTickStatus::Stepped,
                "default physics-off tick stepped") &&
         expect(session.state().transient.lastMovementResultAvailable,
                "default physics-off movement result") &&
         expect(session.state().transient.lastMovementResult.blocked ==
                    iggy3d::MovementBlockedReason::BlockedByCollision,
                "default physics-off uses legacy blocked wall") &&
         expect(!session.state().transient.lastMovementResult.physicsFrameStatsAvailable,
                "default physics-off no stats") &&
         expect(iggy3d::nearlyEqual(session.state().world.findById({1})->transform.position,
                                    {0.0F, 0.0F, 0.0F}),
                "default physics-off no mutation");
}

bool physicsPlannerTickOptionPartiallyMovesAgainstWall() {
  iggy3d::Session session = makeSession();
  const iggy3d::SpatialSurfaceSet surfaces = tickCollisionSurfaces();

  const iggy3d::SessionCommandResult move =
      session.submitCommand(submittedMove({0.0F, 0.0F, -2.0F}));
  bool ok = expect(move.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
                   "physics tick move accepted") &&
            expect(session.tickWithOptions({&surfaces, true}).status ==
                       iggy3d::ResultStatus::Ok,
                   "physics tick option ok") &&
            expect(session.state().transient.pendingExecutionSequences.empty(),
                   "physics tick queue consumed") &&
            expect(session.state().transient.lastMovementResultAvailable,
                   "physics tick movement result available");

  const iggy3d::MovementResult& movement =
      session.state().transient.lastMovementResult;
  const iggy3d::Vec3 finalPosition =
      session.state().world.findById({1})->transform.position;
  ok = ok && expect(movement.blocked == iggy3d::MovementBlockedReason::None,
                    "physics tick movement succeeds with partial clamp") &&
       expect(movement.movementClamped, "physics tick movement clamped") &&
       expect(movement.hitSurfaceId == "tick_wall", "physics tick wall id") &&
       expect(movement.collisionSweepCount >= 1U, "physics tick sweep count") &&
       expect(movement.physicsFrameStatsAvailable,
              "physics tick stats available") &&
       expect(movement.physicsFrameStats.ok, "physics tick stats ok") &&
       expect(movement.physicsFrameStats.sourcePacketCount == 1U,
              "physics tick stats source count") &&
       expect(movement.physicsFrameStats.playerHitCount >= 1U,
              "physics tick stats hit count") &&
       expect(movement.physicsFrameStats.playerIterationCount >= 1U,
              "physics tick stats iterations") &&
       expect(finalPosition.z < -0.10F && finalPosition.z > -0.90F,
              "physics tick partial z before wall") &&
       expect(iggy3d::nearlyEqual(finalPosition, movement.finalPosition),
              "physics tick final transform matches result") &&
       expect(session.state().clock.tickIndex == 1U,
              "physics tick advanced");
  return ok;
}

bool physicsPlannerStepOneTickOptionCompilesAndRuns() {
  iggy3d::Session session = makeSession();
  const iggy3d::SpatialSurfaceSet surfaces = tickCollisionSurfaces();

  const iggy3d::SessionCommandResult move =
      session.submitCommand(submittedMove({0.0F, 0.0F, -2.0F}));
  session.mutableStateForOwnedSystems().clock.mode = iggy3d::ClockMode::Paused;
  session.mutableStateForOwnedSystems().clock.timeScale = 0.0F;
  session.mutableStateForOwnedSystems().clock.stepRequested = true;
  bool ok = expect(move.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
                   "physics step move accepted") &&
            expect(session.stepOneTickWithOptions({&surfaces, true}).status ==
                       iggy3d::ResultStatus::Ok,
                   "physics step option ok") &&
            expect(session.state().transient.lastMovementResultAvailable,
                   "physics step movement result available") &&
            expect(session.state().transient.lastMovementResult.movementClamped,
                   "physics step clamped");
  return ok;
}

bool npcAiTickEnqueuesAttackThroughAdmissionAndCombat() {
  iggy3d::Session session = makeNpcCombatSession();
  seedAlertedNpcAtCombat(session, "default");  // instant combat path (plumbing test)

  bool ok = expect(session.tick().status == iggy3d::ResultStatus::Ok, "npc attack tick ok");
  const iggy3d::CombatantState* playerCombatant =
      findCombatant(session.state().combat, {1});
  const iggy3d::CommandRecord* command =
      lastCommandWithSource(session.state().commandLog, iggy3d::CommandSource::Ai);
  const iggy3d::AiActorState* aiActor = findAiActor(session.state().ai, {2});

  ok = ok && expect(playerCombatant != nullptr && playerCombatant->hitPoints == 9,
                    "npc attack damages player") &&
       expect(command != nullptr && command->kind == iggy3d::CommandKind::Attack,
              "ai attack logged") &&
       expect(command != nullptr &&
                  command->admission == iggy3d::CommandAdmissionStatus::Accepted,
              "ai attack accepted") &&
       expect(command != nullptr && command->playerSlot == iggy3d::kInvalidPlayerSlotId,
              "ai attack keeps invalid player slot") &&
       expect(command != nullptr && command->actor == iggy3d::EntityId{2},
              "ai attack actor") &&
       expect(command != nullptr && command->payload.target.entity == iggy3d::EntityId{1},
              "ai attack target") &&
       expect(aiActor != nullptr &&
                  aiActor->behavior == iggy3d::AiBehaviorKind::Attacking,
              "ai behavior attacking") &&
       expect(aiActor != nullptr &&
                  aiActor->lastIntent == iggy3d::AiIntentKind::AttackTarget,
              "ai intent attack") &&
       expect(aiActor != nullptr && aiActor->target == iggy3d::EntityId{1},
              "ai target player") &&
       expect(aiActor != nullptr && aiActor->cooldownTicksRemaining == 2U,
              "ai attack cooldown set");
  return ok;
}

bool npcAiCooldownTickWaitsWithoutSecondAttack() {
  iggy3d::Session session = makeNpcCombatSession();
  seedAlertedNpcAtCombat(session, "default");  // instant combat path (plumbing test)
  bool ok = expect(session.tick().status == iggy3d::ResultStatus::Ok, "first npc tick ok") &&
            expect(session.tick().status == iggy3d::ResultStatus::Ok, "second npc tick ok");

  const iggy3d::CombatantState* playerCombatant =
      findCombatant(session.state().combat, {1});
  const iggy3d::CommandRecord* command =
      lastCommandWithSource(session.state().commandLog, iggy3d::CommandSource::Ai);
  const iggy3d::AiActorState* aiActor = findAiActor(session.state().ai, {2});

  ok = ok && expect(playerCombatant != nullptr && playerCombatant->hitPoints == 9,
                    "cooldown avoids second attack") &&
       expect(command != nullptr && command->kind == iggy3d::CommandKind::Wait,
              "cooldown emits ai wait") &&
       expect(command != nullptr &&
                  command->admission == iggy3d::CommandAdmissionStatus::Accepted,
              "ai wait accepted") &&
       expect(aiActor != nullptr && aiActor->behavior == iggy3d::AiBehaviorKind::Alert,
              "cooldown behavior alert") &&
       expect(aiActor != nullptr && aiActor->lastIntent == iggy3d::AiIntentKind::Wait,
              "cooldown intent wait") &&
       expect(aiActor != nullptr && aiActor->cooldownTicksRemaining == 1U,
              "cooldown decremented");
  return ok;
}

bool npcAiChaseMovesThroughNormalCommandExecution() {
  iggy3d::Session session = makeNpcCombatSession(4.0F);
  seedAlertedNpcAtCombat(session, "default");  // instant combat path (plumbing test)

  bool ok = expect(session.tick().status == iggy3d::ResultStatus::Ok, "npc chase tick ok");
  const iggy3d::CommandRecord* command =
      lastCommandWithSource(session.state().commandLog, iggy3d::CommandSource::Ai);
  const iggy3d::EntityState* npc = session.state().world.findById({2});
  const iggy3d::AiActorState* aiActor = findAiActor(session.state().ai, {2});

  ok = ok && expect(command != nullptr && command->kind == iggy3d::CommandKind::Move,
                    "ai chase move logged") &&
       expect(command != nullptr &&
                  command->admission == iggy3d::CommandAdmissionStatus::Accepted,
              "ai chase move accepted") &&
       expect(command != nullptr && command->source == iggy3d::CommandSource::Ai,
              "ai chase move source") &&
       expect(command != nullptr && command->playerSlot == iggy3d::kInvalidPlayerSlotId,
              "ai chase move invalid player slot") &&
       expect(command != nullptr && command->payload.target.hasPoint,
              "ai chase move target point") &&
       expect(npc != nullptr &&
                  iggy3d::nearlyEqual(npc->transform.position, {3.0F, 0.0F, 0.0F}),
              "npc moved toward player") &&
       expect(aiActor != nullptr && aiActor->behavior == iggy3d::AiBehaviorKind::Chasing,
              "ai chase behavior") &&
       expect(aiActor != nullptr &&
                  aiActor->lastIntent == iggy3d::AiIntentKind::MoveTowardTarget,
              "ai chase intent") &&
       expect(aiActor != nullptr && aiActor->target == iggy3d::EntityId{1},
              "ai chase target player");
  return ok;
}

bool npcVisionConeGatesSessionPerception() {
  // Seeded facing points at the player, so the NPC perceives and chases, and
  // the AI drives its gaze toward the target.
  iggy3d::Session seeing = makeNpcCombatSession(4.0F);
  seedAlertedNpcAtCombat(seeing, "default");  // instant combat path (plumbing test)
  bool ok = expect(seeing.tick().status == iggy3d::ResultStatus::Ok,
                   "vision see tick ok");
  const iggy3d::AiActorState* seeingAi = findAiActor(seeing.state().ai, {2});
  ok = ok &&
       expect(seeingAi != nullptr && seeingAi->behavior == iggy3d::AiBehaviorKind::Chasing,
              "vision see chases") &&
       expect(seeingAi != nullptr && seeingAi->lastTargetInRadius,
              "vision see in radius") &&
       expect(seeingAi != nullptr && seeingAi->lastTargetInVisionCone,
              "vision see in cone") &&
       expect(seeingAi != nullptr && seeingAi->lastTargetHasLineOfSight,
              "vision see has line of sight") &&
       expect(seeingAi != nullptr &&
                  iggy3d::nearlyEqual(seeingAi->facingDirection, {-1.0F, 0.0F, 0.0F}),
              "vision facing driven toward player");

  // Force the NPC to look away: the player leaves its cone and goes unseen.
  // Seed the AI actor at create time so the facing override lands before the
  // first tick (an unseeded actor is created lazily during the tick).
  iggy3d::Result<iggy3d::Session> blindResult =
      createNpcCombatSessionWithAiSeed("training_npc", "default", 4.0F);
  ok = ok && expect(blindResult.status == iggy3d::ResultStatus::Ok,
                    "vision blind create ok");
  if (blindResult.status == iggy3d::ResultStatus::Ok) {
    iggy3d::Session& blind = blindResult.value;
    iggy3d::SessionState& blindState = blind.mutableStateForOwnedSystems();
    for (iggy3d::AiActorState& actor : blindState.ai.actors) {
      if (actor.actor == iggy3d::EntityId{2}) {
        actor.facingDirection = {1.0F, 0.0F, 0.0F};  // away from player at -x
      }
    }
    ok = ok && expect(blind.tick().status == iggy3d::ResultStatus::Ok,
                      "vision blind tick ok");
    const iggy3d::AiActorState* blindAi = findAiActor(blind.state().ai, {2});
    const iggy3d::CommandRecord* aiCommand =
        lastCommandWithSource(blind.state().commandLog, iggy3d::CommandSource::Ai);
    ok = ok &&
         expect(blindAi != nullptr && blindAi->behavior == iggy3d::AiBehaviorKind::Idle,
                "vision blind idle") &&
         expect(blindAi != nullptr && !blindAi->lastTargetInVisionCone,
                "vision blind out of cone") &&
         expect(aiCommand == nullptr || aiCommand->kind != iggy3d::CommandKind::Move,
                "vision blind no chase command");
  }
  return ok;
}

bool authoredNpcFacingOverridesDefaultAndGatesVision() {
  iggy3d::SessionCreateRequest request;
  request.config = iggy3d::makeDefaultRuntimeConfig();
  request.seed = makeNpcCombatSeed(4.0F);  // npc at +4x, player at origin (west)
  iggy3d::ScenarioAiActorSeed aiSeed;
  aiSeed.actorStableName = "training_npc";
  aiSeed.behaviorProfileId = "melee_training";
  aiSeed.hasFacing = true;
  aiSeed.facingDegrees = 90.0F;  // face +x (east), away from the player to the west
  request.seed.aiActors.push_back(aiSeed);

  const iggy3d::Result<iggy3d::Session> created = iggy3d::Session::create(request);
  if (!expect(created.status == iggy3d::ResultStatus::Ok, "authored facing create ok")) {
    return false;
  }
  iggy3d::Session session = std::move(created.value);
  const iggy3d::AiActorState* seeded = findAiActor(session.state().ai, {2});
  bool ok = expect(seeded != nullptr &&
                       iggy3d::nearlyEqual(seeded->facingDirection, {1.0F, 0.0F, 0.0F}),
                   "authored facing overrides player-facing default");

  // Player is directly behind the authored gaze -> stays unseen.
  ok = ok && expect(session.tick().status == iggy3d::ResultStatus::Ok,
                    "authored facing tick ok");
  const iggy3d::AiActorState* ticked = findAiActor(session.state().ai, {2});
  return ok &&
         expect(ticked != nullptr && !ticked->lastTargetInVisionCone,
                "player behind authored facing is out of cone") &&
         expect(ticked != nullptr && ticked->behavior == iggy3d::AiBehaviorKind::Idle,
                "npc stays unaware of the player behind it");
}

bool sessionCreateSeedsAiActorProfileIntoStateAndBaseline() {
  const iggy3d::Result<iggy3d::Session> created =
      createNpcCombatSessionWithAiSeed("training_npc", "passive");
  if (!expect(created.status == iggy3d::ResultStatus::Ok, "ai seed create ok")) {
    return false;
  }

  const iggy3d::AiActorState* aiActor = findAiActor(created.value.state().ai, {2});
  const iggy3d::AiActorState* baselineAiActor =
      findAiActor(created.value.state().baseline.ai, {2});

  return expect(aiActor != nullptr, "seeded ai actor exists") &&
         expect(aiActor != nullptr && aiActor->behaviorProfileId == "passive",
                "seeded ai actor profile") &&
         expect(aiActor != nullptr && aiActor->behavior == iggy3d::AiBehaviorKind::Idle,
                "seeded ai actor default behavior") &&
         expect(baselineAiActor != nullptr, "baseline ai actor exists") &&
         expect(baselineAiActor != nullptr && baselineAiActor->behaviorProfileId == "passive",
                "baseline ai actor profile");
}

bool sessionCreateRejectsInvalidAiActorSeeds() {
  iggy3d::SessionCreateRequest missingActor;
  missingActor.config = iggy3d::makeDefaultRuntimeConfig();
  missingActor.seed = makeNpcCombatSeed();
  missingActor.seed.aiActors.push_back({"missing_npc", "passive"});
  const iggy3d::Result<iggy3d::Session> missingResult =
      iggy3d::Session::create(missingActor);

  iggy3d::SessionCreateRequest nonNpc;
  nonNpc.config = iggy3d::makeDefaultRuntimeConfig();
  nonNpc.seed = makeNpcCombatSeed();
  nonNpc.seed.aiActors.push_back({"player", "passive"});
  const iggy3d::Result<iggy3d::Session> nonNpcResult =
      iggy3d::Session::create(nonNpc);

  iggy3d::SessionCreateRequest duplicate;
  duplicate.config = iggy3d::makeDefaultRuntimeConfig();
  duplicate.seed = makeNpcCombatSeed();
  duplicate.seed.aiActors.push_back({"training_npc", "passive"});
  duplicate.seed.aiActors.push_back({"training_npc", "default"});
  const iggy3d::Result<iggy3d::Session> duplicateResult =
      iggy3d::Session::create(duplicate);

  return expect(missingResult.status == iggy3d::ResultStatus::Error,
                "missing ai actor rejects") &&
         expect(missingResult.error.code == "session.ai_seed_missing_actor",
                "missing ai actor error") &&
         expect(nonNpcResult.status == iggy3d::ResultStatus::Error,
                "non npc ai actor rejects") &&
         expect(nonNpcResult.error.code == "session.ai_seed_non_npc_actor",
                "non npc ai actor error") &&
         expect(duplicateResult.status == iggy3d::ResultStatus::Error,
                "duplicate ai actor rejects") &&
         expect(duplicateResult.error.code == "session.ai_seed_duplicate_actor",
                "duplicate ai actor error");
}

bool sessionCreateSeedsGuardAnchorIntoStateAndBaseline() {
  iggy3d::SessionCreateRequest request;
  request.config = iggy3d::makeDefaultRuntimeConfig();
  request.seed = makeNpcGuardSeed();
  request.seed.aiActors.push_back({"training_npc", "passive"});
  request.seed.aiGuardAnchors.push_back(
      {"training_npc", "tactical_marker_alpha", 6.0F, 1.0F, 0.25F});
  const iggy3d::Result<iggy3d::Session> created =
      iggy3d::Session::create(request);
  if (!expect(created.status == iggy3d::ResultStatus::Ok,
              "guard seed create ok")) {
    return false;
  }

  const iggy3d::AiActorState* aiActor = findAiActor(created.value.state().ai, {2});
  const iggy3d::AiActorState* baselineAiActor =
      findAiActor(created.value.state().baseline.ai, {2});
  const iggy3d::EntityState* marker =
      created.value.state().world.findByStableName("tactical_marker_alpha");

  return expect(aiActor != nullptr, "guard seeded ai actor exists") &&
         expect(aiActor != nullptr && aiActor->behaviorProfileId == "passive",
                "guard seeded profile preserved") &&
         expect(aiActor != nullptr && aiActor->hasHomePosition,
                "guard seeded home enabled") &&
         expect(aiActor != nullptr && marker != nullptr &&
                    iggy3d::nearlyEqual(aiActor->homePosition,
                                        marker->transform.position),
                "guard seeded home position") &&
         expect(aiActor != nullptr && aiActor->homeStableName == "tactical_marker_alpha",
                "guard seeded home stable name") &&
         expect(aiActor != nullptr && aiActor->leashRadiusMeters == 6.0F,
                "guard seeded leash") &&
         expect(aiActor != nullptr && aiActor->returnRadiusMeters == 1.0F,
                "guard seeded return radius") &&
         expect(aiActor != nullptr && aiActor->homeToleranceMeters == 0.25F,
                "guard seeded tolerance") &&
         expect(baselineAiActor != nullptr && baselineAiActor->hasHomePosition,
                "guard baseline home enabled") &&
         expect(baselineAiActor != nullptr &&
                    baselineAiActor->homeStableName == "tactical_marker_alpha",
                "guard baseline home stable name");
}

bool sessionCreateSeedsGuardAnchorWithoutProfileSeed() {
  iggy3d::SessionCreateRequest request;
  request.config = iggy3d::makeDefaultRuntimeConfig();
  request.seed = makeNpcGuardSeed();
  request.seed.aiGuardAnchors.push_back(
      {"training_npc", "tactical_marker_alpha", 6.0F, 1.0F, 0.25F});
  const iggy3d::Result<iggy3d::Session> created =
      iggy3d::Session::create(request);
  if (!expect(created.status == iggy3d::ResultStatus::Ok,
              "guard only seed create ok")) {
    return false;
  }

  const iggy3d::AiActorState* aiActor = findAiActor(created.value.state().ai, {2});
  return expect(aiActor != nullptr, "guard only ai actor exists") &&
         expect(aiActor != nullptr && aiActor->behaviorProfileId == "default",
                "guard only default profile") &&
         expect(aiActor != nullptr && aiActor->hasHomePosition,
                "guard only home enabled");
}

bool sessionCreateRejectsInvalidGuardAnchorSeeds() {
  iggy3d::SessionCreateRequest missingActor;
  missingActor.config = iggy3d::makeDefaultRuntimeConfig();
  missingActor.seed = makeNpcGuardSeed();
  missingActor.seed.aiGuardAnchors.push_back(
      {"missing_npc", "tactical_marker_alpha", 6.0F, 1.0F, 0.25F});
  const iggy3d::Result<iggy3d::Session> missingActorResult =
      iggy3d::Session::create(missingActor);

  iggy3d::SessionCreateRequest nonNpcActor;
  nonNpcActor.config = iggy3d::makeDefaultRuntimeConfig();
  nonNpcActor.seed = makeNpcGuardSeed();
  nonNpcActor.seed.aiGuardAnchors.push_back(
      {"player", "tactical_marker_alpha", 6.0F, 1.0F, 0.25F});
  const iggy3d::Result<iggy3d::Session> nonNpcActorResult =
      iggy3d::Session::create(nonNpcActor);

  iggy3d::SessionCreateRequest duplicateActor;
  duplicateActor.config = iggy3d::makeDefaultRuntimeConfig();
  duplicateActor.seed = makeNpcGuardSeed();
  duplicateActor.seed.aiGuardAnchors.push_back(
      {"training_npc", "tactical_marker_alpha", 6.0F, 1.0F, 0.25F});
  duplicateActor.seed.aiGuardAnchors.push_back(
      {"training_npc", "tactical_marker_alpha", 6.0F, 1.0F, 0.25F});
  const iggy3d::Result<iggy3d::Session> duplicateActorResult =
      iggy3d::Session::create(duplicateActor);

  iggy3d::SessionCreateRequest missingAnchor;
  missingAnchor.config = iggy3d::makeDefaultRuntimeConfig();
  missingAnchor.seed = makeNpcGuardSeed();
  missingAnchor.seed.aiGuardAnchors.push_back(
      {"training_npc", "missing_marker", 6.0F, 1.0F, 0.25F});
  const iggy3d::Result<iggy3d::Session> missingAnchorResult =
      iggy3d::Session::create(missingAnchor);

  iggy3d::SessionCreateRequest nonMarkerAnchor;
  nonMarkerAnchor.config = iggy3d::makeDefaultRuntimeConfig();
  nonMarkerAnchor.seed = makeNpcGuardSeed();
  nonMarkerAnchor.seed.aiGuardAnchors.push_back(
      {"training_npc", "player", 6.0F, 1.0F, 0.25F});
  const iggy3d::Result<iggy3d::Session> nonMarkerAnchorResult =
      iggy3d::Session::create(nonMarkerAnchor);

  iggy3d::SessionCreateRequest invalidDistances;
  invalidDistances.config = iggy3d::makeDefaultRuntimeConfig();
  invalidDistances.seed = makeNpcGuardSeed();
  invalidDistances.seed.aiGuardAnchors.push_back(
      {"training_npc", "tactical_marker_alpha", 1.0F, 2.0F, 0.25F});
  const iggy3d::Result<iggy3d::Session> invalidDistancesResult =
      iggy3d::Session::create(invalidDistances);

  return expect(missingActorResult.status == iggy3d::ResultStatus::Error,
                "missing guard actor rejects") &&
         expect(missingActorResult.error.code == "session.guard_seed_missing_actor",
                "missing guard actor error") &&
         expect(nonNpcActorResult.status == iggy3d::ResultStatus::Error,
                "non npc guard actor rejects") &&
         expect(nonNpcActorResult.error.code == "session.guard_seed_non_npc_actor",
                "non npc guard actor error") &&
         expect(duplicateActorResult.status == iggy3d::ResultStatus::Error,
                "duplicate guard actor rejects") &&
         expect(duplicateActorResult.error.code == "session.guard_seed_duplicate_actor",
                "duplicate guard actor error") &&
         expect(missingAnchorResult.status == iggy3d::ResultStatus::Error,
                "missing guard anchor rejects") &&
         expect(missingAnchorResult.error.code == "session.guard_seed_missing_anchor",
                "missing guard anchor error") &&
         expect(nonMarkerAnchorResult.status == iggy3d::ResultStatus::Error,
                "non marker guard anchor rejects") &&
         expect(nonMarkerAnchorResult.error.code == "session.guard_seed_non_marker_anchor",
                "non marker guard anchor error") &&
         expect(invalidDistancesResult.status == iggy3d::ResultStatus::Error,
                "invalid guard distances rejects") &&
         expect(invalidDistancesResult.error.code ==
                    "session.guard_seed_invalid_distances",
                "invalid guard distances error");
}

bool passiveProfileSeededBySessionCreateWaitsWithoutDamage() {
  iggy3d::Result<iggy3d::Session> created =
      createNpcCombatSessionWithAiSeed("training_npc", "passive");
  if (!expect(created.status == iggy3d::ResultStatus::Ok,
              "seeded passive create ok")) {
    return false;
  }
  iggy3d::Session& session = created.value;

  bool ok = expect(session.tick().status == iggy3d::ResultStatus::Ok,
                   "seeded passive tick ok");
  const iggy3d::CommandRecord* command =
      lastCommandWithSource(session.state().commandLog, iggy3d::CommandSource::Ai);
  const iggy3d::CombatantState* playerCombatant =
      findCombatant(session.state().combat, {1});
  const iggy3d::AiActorState* aiActor = findAiActor(session.state().ai, {2});

  ok = ok && expect(command != nullptr && command->kind == iggy3d::CommandKind::Wait,
                    "seeded passive wait logged") &&
       expect(command != nullptr &&
                  command->admission == iggy3d::CommandAdmissionStatus::Accepted,
              "seeded passive wait accepted") &&
       expect(playerCombatant != nullptr && playerCombatant->hitPoints == 10,
              "seeded passive no damage") &&
       expect(aiActor != nullptr && aiActor->behaviorProfileId == "passive",
              "seeded passive profile retained") &&
       expect(aiActor != nullptr && aiActor->behavior == iggy3d::AiBehaviorKind::Alert,
              "seeded passive alert") &&
       expect(aiActor != nullptr && aiActor->lastIntent == iggy3d::AiIntentKind::Wait,
              "seeded passive wait intent");
  return ok;
}

bool unknownProfileSeededBySessionCreateFailsClosed() {
  iggy3d::Result<iggy3d::Session> created =
      createNpcCombatSessionWithAiSeed("training_npc", "ghost_profile");
  if (!expect(created.status == iggy3d::ResultStatus::Ok,
              "seeded unknown create ok")) {
    return false;
  }
  iggy3d::Session& session = created.value;

  bool ok = expect(session.tick().status == iggy3d::ResultStatus::Ok,
                   "seeded unknown tick ok");
  const iggy3d::CommandRecord* command =
      lastCommandWithSource(session.state().commandLog, iggy3d::CommandSource::Ai);
  const iggy3d::CombatantState* playerCombatant =
      findCombatant(session.state().combat, {1});
  const iggy3d::AiActorState* aiActor = findAiActor(session.state().ai, {2});

  ok = ok && expect(command == nullptr, "seeded unknown no ai command") &&
       expect(playerCombatant != nullptr && playerCombatant->hitPoints == 10,
              "seeded unknown no damage") &&
       expect(aiActor != nullptr && aiActor->behaviorProfileId == "ghost_profile",
              "seeded unknown profile retained") &&
       expect(aiActor != nullptr && aiActor->behavior == iggy3d::AiBehaviorKind::Idle,
              "seeded unknown behavior unchanged") &&
       expect(aiActor != nullptr && aiActor->lastIntent == iggy3d::AiIntentKind::None,
              "seeded unknown intent unchanged");
  return ok;
}

bool passiveNpcInAttackRangeWaitsWithoutDamage() {
  iggy3d::Session session = makeNpcCombatSession();
  seedNpcAiProfile(session, "passive");

  bool ok = expect(session.tick().status == iggy3d::ResultStatus::Ok,
                   "passive attack range tick ok");
  const iggy3d::CommandRecord* command =
      lastCommandWithSource(session.state().commandLog, iggy3d::CommandSource::Ai);
  const iggy3d::CombatantState* playerCombatant =
      findCombatant(session.state().combat, {1});
  const iggy3d::AiActorState* aiActor = findAiActor(session.state().ai, {2});

  ok = ok && expect(command != nullptr && command->kind == iggy3d::CommandKind::Wait,
                    "passive attack range wait logged") &&
       expect(command != nullptr &&
                  command->admission == iggy3d::CommandAdmissionStatus::Accepted,
              "passive attack range wait accepted") &&
       expect(playerCombatant != nullptr && playerCombatant->hitPoints == 10,
              "passive attack range no damage") &&
       expect(aiActor != nullptr && aiActor->behaviorProfileId == "passive",
              "passive attack range profile retained") &&
       expect(aiActor != nullptr && aiActor->behavior == iggy3d::AiBehaviorKind::Alert,
              "passive attack range alert") &&
       expect(aiActor != nullptr && aiActor->lastIntent == iggy3d::AiIntentKind::Wait,
              "passive attack range wait intent") &&
       expect(aiActor != nullptr && aiActor->target == iggy3d::EntityId{1},
              "passive attack range target player");
  return ok;
}

bool passiveNpcOutsideAttackRangeWaitsWithoutChasing() {
  iggy3d::Session session = makeNpcCombatSession(4.0F);
  seedNpcAiProfile(session, "passive");

  bool ok = expect(session.tick().status == iggy3d::ResultStatus::Ok,
                   "passive chase range tick ok");
  const iggy3d::CommandRecord* command =
      lastCommandWithSource(session.state().commandLog, iggy3d::CommandSource::Ai);
  const iggy3d::EntityState* npc = session.state().world.findById({2});
  const iggy3d::CombatantState* playerCombatant =
      findCombatant(session.state().combat, {1});
  const iggy3d::AiActorState* aiActor = findAiActor(session.state().ai, {2});

  ok = ok && expect(command != nullptr && command->kind == iggy3d::CommandKind::Wait,
                    "passive chase range wait logged") &&
       expect(command != nullptr &&
                  command->admission == iggy3d::CommandAdmissionStatus::Accepted,
              "passive chase range wait accepted") &&
       expect(npc != nullptr &&
                  iggy3d::nearlyEqual(npc->transform.position, {4.0F, 0.0F, 0.0F}),
              "passive chase range no movement") &&
       expect(playerCombatant != nullptr && playerCombatant->hitPoints == 10,
              "passive chase range no damage") &&
       expect(aiActor != nullptr && aiActor->behavior == iggy3d::AiBehaviorKind::Alert,
              "passive chase range alert") &&
       expect(aiActor != nullptr && aiActor->lastIntent == iggy3d::AiIntentKind::Wait,
              "passive chase range wait intent") &&
       expect(aiActor != nullptr && aiActor->target == iggy3d::EntityId{1},
              "passive chase range target player");
  return ok;
}

bool unknownProfileSkipsNpcCommandAndStateMutation() {
  iggy3d::Session session = makeNpcCombatSession();
  seedNpcAiProfile(session, "ghost_profile");

  bool ok = expect(session.tick().status == iggy3d::ResultStatus::Ok,
                   "unknown profile tick ok");
  const iggy3d::CommandRecord* command =
      lastCommandWithSource(session.state().commandLog, iggy3d::CommandSource::Ai);
  const iggy3d::CombatantState* playerCombatant =
      findCombatant(session.state().combat, {1});
  const iggy3d::EntityState* npc = session.state().world.findById({2});
  const iggy3d::AiActorState* aiActor = findAiActor(session.state().ai, {2});

  ok = ok && expect(command == nullptr, "unknown profile no ai command") &&
       expect(playerCombatant != nullptr && playerCombatant->hitPoints == 10,
              "unknown profile no damage") &&
       expect(npc != nullptr &&
                  iggy3d::nearlyEqual(npc->transform.position, {1.0F, 0.0F, 0.0F}),
              "unknown profile no movement") &&
       expect(aiActor != nullptr && aiActor->behaviorProfileId == "ghost_profile",
              "unknown profile id retained") &&
       expect(aiActor != nullptr && aiActor->behavior == iggy3d::AiBehaviorKind::Idle,
              "unknown profile behavior unchanged") &&
       expect(aiActor != nullptr && aiActor->lastIntent == iggy3d::AiIntentKind::None,
              "unknown profile intent unchanged") &&
       expect(aiActor != nullptr && !iggy3d::isValid(aiActor->target),
              "unknown profile target unchanged");
  return ok;
}

bool invalidProfileSkipsNpcCommandAndStateMutation() {
  iggy3d::Session session = makeNpcCombatSession(4.0F);
  seedNpcAiProfile(session, "Bad-Id");

  bool ok = expect(session.tick().status == iggy3d::ResultStatus::Ok,
                   "invalid profile tick ok");
  const iggy3d::CommandRecord* command =
      lastCommandWithSource(session.state().commandLog, iggy3d::CommandSource::Ai);
  const iggy3d::CombatantState* playerCombatant =
      findCombatant(session.state().combat, {1});
  const iggy3d::EntityState* npc = session.state().world.findById({2});
  const iggy3d::AiActorState* aiActor = findAiActor(session.state().ai, {2});

  ok = ok && expect(command == nullptr, "invalid profile no ai command") &&
       expect(playerCombatant != nullptr && playerCombatant->hitPoints == 10,
              "invalid profile no damage") &&
       expect(npc != nullptr &&
                  iggy3d::nearlyEqual(npc->transform.position, {4.0F, 0.0F, 0.0F}),
              "invalid profile no movement") &&
       expect(aiActor != nullptr && aiActor->behaviorProfileId == "Bad-Id",
              "invalid profile id retained") &&
       expect(aiActor != nullptr && aiActor->behavior == iggy3d::AiBehaviorKind::Idle,
              "invalid profile behavior unchanged") &&
       expect(aiActor != nullptr && aiActor->lastIntent == iggy3d::AiIntentKind::None,
              "invalid profile intent unchanged") &&
       expect(aiActor != nullptr && !iggy3d::isValid(aiActor->target),
              "invalid profile target unchanged");
  return ok;
}

bool forceActorAtCombat(iggy3d::Session& session, iggy3d::EntityId actor) {
  bool found = false;
  for (iggy3d::AiActorState& a : session.mutableStateForOwnedSystems().ai.actors) {
    if (a.actor == actor) {
      a.alertLevel = 1.0F;
      a.lastRiseTick = 0;
      found = true;
    }
  }
  return found;
}

bool autoRegisteredNpcUsesDefaultProfileAndAttacks() {
  iggy3d::Session session = makeNpcCombatSession();

  // Tick 1 proves lazy auto-registration under the default profile; graded alert
  // is still sub-combat so it only emits a Wait. Force alert to combat, then tick
  // 2 exercises the attack plumbing.
  bool ok = expect(session.state().ai.actors.empty(), "auto default starts without ai actor") &&
            expect(session.tick().status == iggy3d::ResultStatus::Ok,
                   "auto default tick ok");
  const iggy3d::AiActorState* registered = findAiActor(session.state().ai, {2});
  ok = ok && expect(registered != nullptr && registered->behaviorProfileId == "default",
                    "auto default profile id") &&
       expect(forceActorAtCombat(session, {2}), "auto default force combat alert") &&
       expect(session.tick().status == iggy3d::ResultStatus::Ok,
              "auto default second tick ok");

  const iggy3d::CommandRecord* command =
      lastCommandWithSource(session.state().commandLog, iggy3d::CommandSource::Ai);
  const iggy3d::CombatantState* playerCombatant =
      findCombatant(session.state().combat, {1});
  const iggy3d::AiActorState* aiActor = findAiActor(session.state().ai, {2});

  ok = ok && expect(command != nullptr && command->kind == iggy3d::CommandKind::Attack,
                    "auto default attack logged") &&
       expect(command != nullptr &&
                  command->admission == iggy3d::CommandAdmissionStatus::Accepted,
              "auto default attack accepted") &&
       expect(playerCombatant != nullptr && playerCombatant->hitPoints == 9,
              "auto default damages player") &&
       expect(aiActor != nullptr && aiActor->behavior == iggy3d::AiBehaviorKind::Attacking,
              "auto default behavior attacking") &&
       expect(aiActor != nullptr &&
                  aiActor->lastIntent == iggy3d::AiIntentKind::AttackTarget,
              "auto default attack intent");
  return ok;
}

bool rejectedAiAttackRemainsVisibleInCommandLog() {
  iggy3d::Session session = makeNpcCombatSession(1.0F, false);

  // Tick 1 registers the actor (sub-combat Wait); force alert to combat, then
  // tick 2 produces the attack that combat admission rejects (invalid target).
  bool ok = expect(session.tick().status == iggy3d::ResultStatus::Ok,
                   "rejected ai attack register tick ok") &&
            expect(forceActorAtCombat(session, {2}), "rejected ai attack force combat") &&
            expect(session.tick().status == iggy3d::ResultStatus::Ok,
                   "rejected ai attack tick ok");
  const iggy3d::CommandRecord* command =
      lastCommandWithSource(session.state().commandLog, iggy3d::CommandSource::Ai);
  const iggy3d::CombatantState* playerCombatant =
      findCombatant(session.state().combat, {1});

  ok = ok && expect(command != nullptr && command->kind == iggy3d::CommandKind::Attack,
                    "rejected ai attack logged") &&
       expect(command != nullptr &&
                  command->admission == iggy3d::CommandAdmissionStatus::Rejected,
              "rejected ai attack status") &&
       expect(command != nullptr &&
                  command->rejection == iggy3d::CommandRejectionReason::InvalidTarget,
              "rejected ai attack reason") &&
       expect(command != nullptr && command->playerSlot == iggy3d::kInvalidPlayerSlotId,
              "rejected ai attack invalid player slot") &&
       expect(playerCombatant != nullptr && playerCombatant->hitPoints == 10,
              "rejected ai attack no damage");
  return ok;
}

bool defeatedPlayerIsNotAttackedAgain() {
  iggy3d::Session session = makeNpcCombatSession();
  iggy3d::CombatantState* playerCombatant =
      findMutableCombatant(session.mutableStateForOwnedSystems().combat, {1});
  if (playerCombatant == nullptr) {
    return expect(false, "missing player combatant");
  }
  playerCombatant->hitPoints = 0;
  playerCombatant->defeated = true;

  bool ok = expect(session.tick().status == iggy3d::ResultStatus::Ok,
                   "defeated player tick ok");
  const iggy3d::CommandRecord* command =
      lastCommandWithSource(session.state().commandLog, iggy3d::CommandSource::Ai);
  const iggy3d::AiActorState* aiActor = findAiActor(session.state().ai, {2});
  const iggy3d::CombatantState* playerAfter =
      findCombatant(session.state().combat, {1});

  ok = ok && expect(command == nullptr, "defeated player no ai command") &&
       expect(playerAfter != nullptr && playerAfter->hitPoints == 0 && playerAfter->defeated,
              "defeated player unchanged") &&
       expect(aiActor != nullptr && aiActor->behavior != iggy3d::AiBehaviorKind::Attacking,
              "defeated player not attacking behavior") &&
       expect(aiActor != nullptr && aiActor->lastIntent == iggy3d::AiIntentKind::Wait,
              "defeated player wait intent");
  return ok;
}

bool defeatedNpcDoesNotEnqueueAttackOrMove() {
  iggy3d::Session session = makeNpcCombatSession();
  iggy3d::CombatantState* npcCombatant =
      findMutableCombatant(session.mutableStateForOwnedSystems().combat, {2});
  if (npcCombatant == nullptr) {
    return expect(false, "missing npc combatant");
  }
  npcCombatant->hitPoints = 0;
  npcCombatant->defeated = true;

  bool ok = expect(session.tick().status == iggy3d::ResultStatus::Ok, "defeated npc tick ok");
  const iggy3d::CommandRecord* command =
      lastCommandWithSource(session.state().commandLog, iggy3d::CommandSource::Ai);
  const iggy3d::AiActorState* aiActor = findAiActor(session.state().ai, {2});

  ok = ok && expect(command == nullptr, "defeated npc does not command") &&
       expect(aiActor != nullptr && aiActor->behavior == iggy3d::AiBehaviorKind::Defeated,
              "defeated npc behavior") &&
       expect(aiActor != nullptr && aiActor->lastIntent == iggy3d::AiIntentKind::None,
              "defeated npc intent none");
  return ok;
}

bool pausedNormalTickDoesNotRunNpcAi() {
  iggy3d::Session session = makeNpcCombatSession();
  session.mutableStateForOwnedSystems().clock.mode = iggy3d::ClockMode::Paused;

  bool ok = expect(session.tick().status == iggy3d::ResultStatus::Error,
                   "paused tick rejected");
  const iggy3d::CommandRecord* command =
      lastCommandWithSource(session.state().commandLog, iggy3d::CommandSource::Ai);
  const iggy3d::AiActorState* aiActor = findAiActor(session.state().ai, {2});
  ok = ok && expect(command == nullptr, "paused tick no ai command") &&
       expect(aiActor == nullptr, "paused tick does not auto-register ai");
  return ok;
}

// The in-loop proof (not just the pure FSM): a perceived hostile NPC climbs the
// alert ladder over ticks and eventually chases, then decays once it loses sight.
// This is the one test intentionally coupled to the AlertProfile tuning.
bool npcAlertLadderEscalatesThenDecaysInLoop() {
  // Player at origin, NPC at x=3 (outside attack range 1.5, so combat==Chasing).
  iggy3d::Session session = makeNpcCombatSession(3.0F);
  seedNpcAiProfile(session, "default");  // facing -x -> player perceived from tick 1
  const iggy3d::AlertProfile profile;    // matches the built-in default profile

  bool ok = true;
  std::uint8_t maxBandSeen = 0U;
  std::uint8_t lastBand = 0U;
  bool sawObservant = false;
  bool sawSuspicious = false;
  bool sawSearching = false;
  bool sawAlert = false;
  bool reachedChasing = false;
  constexpr int kTickCap = 200;
  int climbTicks = 0;
  for (; climbTicks < kTickCap; ++climbTicks) {
    if (session.tick().status != iggy3d::ResultStatus::Ok) {
      ok = expect(false, "escalation climb tick ok");
      break;
    }
    const iggy3d::AiActorState* actor = findAiActor(session.state().ai, {2});
    if (actor == nullptr) {
      ok = expect(false, "escalation actor present");
      break;
    }
    const std::uint8_t band = iggy3d::alertBandIndex(actor->alertLevel, profile);
    // Monotonic non-decreasing while perceived (no decay path taken here).
    ok = ok && expect(band >= lastBand, "escalation band non-decreasing");
    lastBand = band;
    maxBandSeen = band > maxBandSeen ? band : maxBandSeen;
    switch (actor->behavior) {
      case iggy3d::AiBehaviorKind::Observant: sawObservant = true; break;
      case iggy3d::AiBehaviorKind::Suspicious: sawSuspicious = true; break;
      case iggy3d::AiBehaviorKind::Searching: sawSearching = true; break;
      case iggy3d::AiBehaviorKind::Alert: sawAlert = true; break;
      case iggy3d::AiBehaviorKind::Chasing: reachedChasing = true; break;
      default: break;
    }
    if (reachedChasing) {
      break;
    }
  }

  ok = ok && expect(sawObservant, "escalation passed through observant") &&
       expect(sawSuspicious, "escalation passed through suspicious") &&
       expect(sawSearching, "escalation passed through searching") &&
       expect(sawAlert, "escalation passed through alert") &&
       expect(reachedChasing, "escalation reached chasing") &&
       expect(maxBandSeen == 5U, "escalation reached combat band");

  // Decay: turn the NPC away so the player leaves its cone (perception != Ready).
  const iggy3d::AiActorState* atPeak = findAiActor(session.state().ai, {2});
  const float peak = atPeak != nullptr ? atPeak->alertLevel : 0.0F;
  for (iggy3d::AiActorState& a : session.mutableStateForOwnedSystems().ai.actors) {
    if (a.actor == iggy3d::EntityId{2}) {
      a.facingDirection = {1.0F, 0.0F, 0.0F};  // away from player at -x
    }
  }
  // Tick past dead-time plus a margin; the agitated band drains slowly by design,
  // so we assert strict decrease (direction), not full return to Idle. A blind NPC
  // enqueues no command, and a tick with no work does not advance the clock (so the
  // dead-time would never elapse); submit a player Wait each tick to keep the clock
  // advancing, exactly as ongoing player activity would in a live session.
  const int kDecayTicks = static_cast<int>(profile.deadTimeTicks) + 40;
  for (int i = 0; i < kDecayTicks; ++i) {
    (void)session.submitCommand(submittedWait());
    ok = ok && expect(session.tick().status == iggy3d::ResultStatus::Ok,
                      "escalation decay tick ok");
  }
  const iggy3d::AiActorState* decayed = findAiActor(session.state().ai, {2});
  ok = ok && expect(decayed != nullptr && decayed->alertLevel < peak,
                    "escalation decays back down") &&
       expect(decayed != nullptr &&
                  decayed->behavior != iggy3d::AiBehaviorKind::Chasing,
              "escalation no longer chasing after decay");
  return ok;
}

// (a) An NPC with an authored loop route walks the waypoints in order and wraps. It never
// perceives the player (faces its waypoints, which lie on the +z line away from the origin),
// so it stays low-alert and patrols every tick.
bool patrolNpcWalksLoopRouteInOrder() {
  iggy3d::Session session = makeNpcCombatSession(4.0F);  // npc at (4,0,0), player at origin
  seedPatrolNpc(session, {{4.0F, 0.0F, 3.0F}, {4.0F, 0.0F, -3.0F}},
                iggy3d::PatrolMode::Loop);

  bool ok = true;
  float maxZ = 0.0F;
  float minZ = 0.0F;
  bool sawIndex1 = false;
  bool sawWrapBackTo0 = false;
  bool patrolEveryTick = true;
  for (int tick = 0; tick < 40; ++tick) {
    ok = ok && expect(session.tick().status == iggy3d::ResultStatus::Ok, "patrol tick ok");
    const iggy3d::AiActorState* ai = findAiActor(session.state().ai, {2});
    const iggy3d::EntityState* npc = session.state().world.findById({2});
    if (ai == nullptr || npc == nullptr) {
      return expect(false, "patrol actor/entity present");
    }
    patrolEveryTick = patrolEveryTick && ai->lastIntent == iggy3d::AiIntentKind::Patrol;
    maxZ = std::max(maxZ, npc->transform.position.z);
    minZ = std::min(minZ, npc->transform.position.z);
    if (ai->patrolTargetIndex == 1U) {
      sawIndex1 = true;
    }
    if (sawIndex1 && ai->patrolTargetIndex == 0U) {
      sawWrapBackTo0 = true;  // reached the 2nd waypoint, looped back to the 1st
    }
  }

  ok = ok && expect(patrolEveryTick, "patrol intent every tick") &&
       expect(maxZ >= 2.7F, "patrol reached first waypoint (+z)") &&
       expect(minZ <= -2.7F, "patrol reached second waypoint (-z)") &&
       expect(sawIndex1, "patrol advanced to second waypoint") &&
       expect(sawWrapBackTo0, "patrol looped back to first waypoint");
  return ok;
}

// s6c: a guard on a rectangular (diagonal-cornered) loop must visit ALL FOUR corners in order
// and complete a full lap -- never stall at a corner. This square STALLS under the pre-fix
// design (patrol move-stop == arrival-epsilon: a cornered approach parks a float hair outside
// the arrival ring). The route sits far from the origin so the player is out of perception
// radius and the guard stays low-alert and patrols the whole time.
bool patrolNpcLapsRectangularRouteWithDiagonalCorners() {
  iggy3d::Session session = makeNpcCombatSession(8.0F);  // npc at (8,0,0), player at origin
  const std::vector<iggy3d::Vec3> square = {
      {8.0F, 0.0F, 3.0F}, {11.0F, 0.0F, 3.0F}, {11.0F, 0.0F, 6.0F}, {8.0F, 0.0F, 6.0F}};
  seedPatrolNpc(session, square, iggy3d::PatrolMode::Loop);

  bool ok = true;
  bool visited[4] = {false, false, false, false};
  int lastIndex = 0;
  bool inOrder = true;
  bool completedLap = false;
  bool patrolEveryTick = true;
  // Perimeter 12 m at ~1 m/tick + the spawn approach, laps in ~15 ticks; give ~2 laps of slack.
  for (int tick = 0; tick < 60 && !completedLap; ++tick) {
    ok = ok && expect(session.tick().status == iggy3d::ResultStatus::Ok, "square patrol tick ok");
    const iggy3d::AiActorState* ai = findAiActor(session.state().ai, {2});
    if (ai == nullptr) {
      return expect(false, "square patrol actor present");
    }
    patrolEveryTick = patrolEveryTick && ai->lastIntent == iggy3d::AiIntentKind::Patrol;
    const int idx = static_cast<int>(ai->patrolTargetIndex);
    // The cursor targets the NEXT corner; it advances 0->1->2->3->0. Track order + wrap.
    if (idx != lastIndex) {
      const int expected = (lastIndex + 1) % 4;
      if (idx != expected) {
        inOrder = false;
      }
      if (lastIndex == 3 && idx == 0 && visited[0] && visited[1] && visited[2]) {
        completedLap = true;  // came back to the start after touching every corner
      }
      lastIndex = idx;
    }
    visited[idx] = true;
  }

  ok = ok && expect(patrolEveryTick, "square patrol intent every tick") &&
       expect(visited[0] && visited[1] && visited[2] && visited[3],
              "square patrol visits all four corners") &&
       expect(inOrder, "square patrol advances corners in order (no stall/skip)") &&
       expect(completedLap, "square patrol completes a full lap");
  return ok;
}

// (b) COMPOSE with s5: a patrolling NPC that perceives the player escalates and STOPS
// patrolling; once alert decays back to the low band it RESUMES. Asserts on alert band +
// lastIntent (patrol facing interacts with the cone, so positions are not asserted here).
bool patrolYieldsToEscalationThenResumes() {
  iggy3d::Session session = makeNpcCombatSession(4.0F);
  seedPatrolNpc(session, {{4.0F, 0.0F, 3.0F}, {4.0F, 0.0F, -3.0F}},
                iggy3d::PatrolMode::Loop);

  // Patrolling while unaware.
  bool ok = expect(session.tick().status == iggy3d::ResultStatus::Ok, "compose patrol tick ok");
  const iggy3d::AiActorState* ai = findAiActor(session.state().ai, {2});
  ok = ok && expect(ai != nullptr && ai->lastIntent == iggy3d::AiIntentKind::Patrol,
                    "compose starts patrolling");

  // Escalate: face the player and jump alert into the Suspicious band. Patrol must yield.
  iggy3d::AiActorState* mutableAi = mutableAiActor(session, {2});
  if (mutableAi == nullptr) {
    return expect(false, "compose mutable actor");
  }
  mutableAi->facingDirection = {-1.0F, 0.0F, 0.0F};  // toward player at origin
  mutableAi->alertLevel = 0.30F;                     // Suspicious band (>0.26)
  ok = ok && expect(session.tick().status == iggy3d::ResultStatus::Ok, "compose escalate tick ok");
  ai = findAiActor(session.state().ai, {2});
  ok = ok && expect(ai != nullptr && ai->lastIntent != iggy3d::AiIntentKind::Patrol,
                    "escalation stops patrol") &&
       expect(ai != nullptr && ai->behavior == iggy3d::AiBehaviorKind::Suspicious,
              "escalation raises behavior to suspicious");

  // Look away and let alert decay; patrol resumes once back in the low band. A blind NPC
  // enqueues no command, so submit a player Wait each tick to keep the clock advancing.
  mutableAi = mutableAiActor(session, {2});
  mutableAi->facingDirection = {0.0F, 0.0F, 1.0F};  // +z: player out of cone
  bool resumed = false;
  for (int tick = 0; tick < 300 && !resumed; ++tick) {
    (void)session.submitCommand(submittedWait());
    ok = ok && expect(session.tick().status == iggy3d::ResultStatus::Ok, "compose decay tick ok");
    const iggy3d::AiActorState* decayAi = findAiActor(session.state().ai, {2});
    if (decayAi != nullptr && decayAi->lastIntent == iggy3d::AiIntentKind::Patrol) {
      resumed = true;
    }
  }
  ok = ok && expect(resumed, "patrol resumes after alert decays to low band");
  return ok;
}

// (c) BACK-COMPAT: an NPC with NO authored waypoints does not move while idle, exactly as
// before this slice.
bool idleNpcWithoutWaypointsDoesNotMove() {
  iggy3d::Session session = makeNpcCombatSession(4.0F);
  seedPatrolNpc(session, {}, iggy3d::PatrolMode::Loop);  // empty route: no patrol

  const iggy3d::EntityState* before = session.state().world.findById({2});
  const iggy3d::Vec3 startPos = before->transform.position;

  bool ok = true;
  for (int tick = 0; tick < 8; ++tick) {
    (void)session.submitCommand(submittedWait());
    ok = ok && expect(session.tick().status == iggy3d::ResultStatus::Ok, "idle tick ok");
  }
  const iggy3d::AiActorState* ai = findAiActor(session.state().ai, {2});
  const iggy3d::EntityState* after = session.state().world.findById({2});
  ok = ok && expect(ai != nullptr && ai->lastIntent != iggy3d::AiIntentKind::Patrol,
                    "no-waypoint npc does not patrol") &&
       expect(after != nullptr && iggy3d::nearlyEqual(after->transform.position, startPos),
              "no-waypoint npc stays put while idle");
  return ok;
}

// s7: a guard that loses a target it was looking at records the last-known position, walks there
// to investigate (Searching band), looks around for the bounded dwell, and gives up (memory
// cleared) — it does NOT instantly reset. Uses the white-box seam to seed the Searching band and
// to break contact deterministically.
bool guardInvestigatesLastKnownThenGivesUp() {
  iggy3d::Session session = makeNpcCombatSession(4.0F);  // guard {2} at (4,0,0), player {1} at origin
  seedNpcAiProfile(session, "default");                  // guard faces -x toward the player
  for (iggy3d::AiActorState& a : session.mutableStateForOwnedSystems().ai.actors) {
    if (a.actor == iggy3d::EntityId{2}) {
      a.alertLevel = 0.50F;  // Searching band (0.43 <= level < 0.78)
      a.lastRiseTick = 0;
    }
  }

  // (a) See the player -> record last-known at the sighting (the player's position, origin).
  (void)session.submitCommand(submittedWait());
  bool ok = expect(session.tick().status == iggy3d::ResultStatus::Ok, "investigate see tick ok");
  const iggy3d::AiActorState* ai = findAiActor(session.state().ai, {2});
  ok = ok && expect(ai != nullptr && ai->hasLastKnownTarget, "records last-known while seen") &&
       expect(ai != nullptr &&
                  iggy3d::nearlyEqual(ai->lastKnownTargetPosition, {0.0F, 0.0F, 0.0F}, 0.05F),
              "last-known is the sighting position");

  // Break contact: teleport the player far outside perception so the guard loses sight.
  {
    iggy3d::WorldState& world = session.mutableStateForOwnedSystems().world;
    const iggy3d::EntityState* p = world.findById({1});
    iggy3d::EntityState copy = *p;
    copy.transform.position = {50.0F, 0.0F, 50.0F};
    (void)world.upsertEntity(copy);
  }

  // (b) Now unseen but still Searching -> investigate: walk toward last-known (origin, -x).
  bool sawInvestigate = false;
  float startX = 4.0F;
  for (int i = 0; i < 8; ++i) {
    (void)session.submitCommand(submittedWait());
    ok = ok && expect(session.tick().status == iggy3d::ResultStatus::Ok, "investigate move tick ok");
    ai = findAiActor(session.state().ai, {2});
    if (ai != nullptr && ai->lastIntent == iggy3d::AiIntentKind::Investigate) {
      sawInvestigate = true;
    }
  }
  const iggy3d::EntityState* npc = session.state().world.findById({2});
  ok = ok && expect(sawInvestigate, "guard investigates toward last-known") &&
       expect(npc != nullptr && npc->transform.position.x < startX - 1.0F,
              "guard advanced toward last-known");

  // (c) Arrive and dwell: intent becomes Wait and the look-around counter climbs.
  bool sawDwell = false;
  for (int i = 0; i < 20 && !sawDwell; ++i) {
    (void)session.submitCommand(submittedWait());
    ok = ok && expect(session.tick().status == iggy3d::ResultStatus::Ok, "investigate dwell tick ok");
    ai = findAiActor(session.state().ai, {2});
    if (ai != nullptr && ai->hasLastKnownTarget && ai->investigateDwellTicks > 0U &&
        ai->lastIntent == iggy3d::AiIntentKind::Wait) {
      sawDwell = true;
    }
  }
  ok = ok && expect(sawDwell, "guard dwells (looks around) at last-known");

  // (d) Give up: after the dwell elapses, memory clears and it stops investigating.
  bool gaveUp = false;
  for (int i = 0; i < static_cast<int>(iggy3d::kInvestigateDwellTicks) + 10 && !gaveUp; ++i) {
    (void)session.submitCommand(submittedWait());
    ok = ok && expect(session.tick().status == iggy3d::ResultStatus::Ok, "investigate giveup tick ok");
    ai = findAiActor(session.state().ai, {2});
    if (ai != nullptr && !ai->hasLastKnownTarget) {
      gaveUp = true;
    }
  }
  ai = findAiActor(session.state().ai, {2});
  ok = ok && expect(gaveUp, "guard gives up after the dwell") &&
       expect(ai != nullptr && ai->lastIntent != iggy3d::AiIntentKind::Investigate,
              "guard no longer investigating after give-up");
  return ok;
}

}  // namespace

int main() {
  const bool ok = tickMovesThenRetryPicksUpKeyExactlyOnce() &&
                  exitObjectiveCompletionSetsVictoryAfterRequiredItems() &&
                  collisionBlockedMoveConsumesPendingCommandWithoutMutation() &&
                  sessionTickInputPhysicsPlannerDefaultsOff() &&
                  physicsPlannerTickOptionPartiallyMovesAgainstWall() &&
                  physicsPlannerStepOneTickOptionCompilesAndRuns() &&
                  npcAiTickEnqueuesAttackThroughAdmissionAndCombat() &&
                  npcAiCooldownTickWaitsWithoutSecondAttack() &&
                  npcAiChaseMovesThroughNormalCommandExecution() &&
                  npcVisionConeGatesSessionPerception() &&
                  authoredNpcFacingOverridesDefaultAndGatesVision() &&
                  sessionCreateSeedsAiActorProfileIntoStateAndBaseline() &&
                  sessionCreateRejectsInvalidAiActorSeeds() &&
                  sessionCreateSeedsGuardAnchorIntoStateAndBaseline() &&
                  sessionCreateSeedsGuardAnchorWithoutProfileSeed() &&
                  sessionCreateRejectsInvalidGuardAnchorSeeds() &&
                  passiveProfileSeededBySessionCreateWaitsWithoutDamage() &&
                  unknownProfileSeededBySessionCreateFailsClosed() &&
                  passiveNpcInAttackRangeWaitsWithoutDamage() &&
                  passiveNpcOutsideAttackRangeWaitsWithoutChasing() &&
                  unknownProfileSkipsNpcCommandAndStateMutation() &&
                  invalidProfileSkipsNpcCommandAndStateMutation() &&
                  autoRegisteredNpcUsesDefaultProfileAndAttacks() &&
                  rejectedAiAttackRemainsVisibleInCommandLog() &&
                  npcAlertLadderEscalatesThenDecaysInLoop() &&
                  patrolNpcWalksLoopRouteInOrder() &&
                  patrolNpcLapsRectangularRouteWithDiagonalCorners() &&
                  patrolYieldsToEscalationThenResumes() &&
                  idleNpcWithoutWaypointsDoesNotMove() &&
                  guardInvestigatesLastKnownThenGivesUp() &&
                  defeatedPlayerIsNotAttackedAgain() &&
                  defeatedNpcDoesNotEnqueueAttackOrMove() &&
                  pausedNormalTickDoesNotRunNpcAi();
  return ok ? 0 : 1;
}
