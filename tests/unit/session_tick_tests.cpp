#include "runtime/session/Session.hpp"

#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/inventory/InventorySystem.hpp"
#include "runtime/objective/ObjectiveSystem.hpp"

#include <iostream>
#include <string>
#include <string_view>

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
  auto& actors = session.mutableStateForOwnedSystems().ai.actors;
  actors.clear();
  actors.push_back(actor);
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

bool npcAiTickEnqueuesAttackThroughAdmissionAndCombat() {
  iggy3d::Session session = makeNpcCombatSession();

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

bool autoRegisteredNpcUsesDefaultProfileAndAttacks() {
  iggy3d::Session session = makeNpcCombatSession();

  bool ok = expect(session.state().ai.actors.empty(), "auto default starts without ai actor") &&
            expect(session.tick().status == iggy3d::ResultStatus::Ok,
                   "auto default tick ok");
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
       expect(aiActor != nullptr && aiActor->behaviorProfileId == "default",
              "auto default profile id") &&
       expect(aiActor != nullptr && aiActor->behavior == iggy3d::AiBehaviorKind::Attacking,
              "auto default behavior attacking") &&
       expect(aiActor != nullptr &&
                  aiActor->lastIntent == iggy3d::AiIntentKind::AttackTarget,
              "auto default attack intent");
  return ok;
}

bool rejectedAiAttackRemainsVisibleInCommandLog() {
  iggy3d::Session session = makeNpcCombatSession(1.0F, false);

  bool ok = expect(session.tick().status == iggy3d::ResultStatus::Ok,
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

}  // namespace

int main() {
  const bool ok = tickMovesThenRetryPicksUpKeyExactlyOnce() &&
                  exitObjectiveCompletionSetsVictoryAfterRequiredItems() &&
                  collisionBlockedMoveConsumesPendingCommandWithoutMutation() &&
                  npcAiTickEnqueuesAttackThroughAdmissionAndCombat() &&
                  npcAiCooldownTickWaitsWithoutSecondAttack() &&
                  npcAiChaseMovesThroughNormalCommandExecution() &&
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
                  defeatedPlayerIsNotAttackedAgain() &&
                  defeatedNpcDoesNotEnqueueAttackOrMove() &&
                  pausedNormalTickDoesNotRunNpcAi();
  return ok ? 0 : 1;
}
