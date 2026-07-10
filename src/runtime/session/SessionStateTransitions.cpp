#include "runtime/session/SessionInternal.hpp"

#include <algorithm>
#include <cmath>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "config/RuntimeConfig.hpp"
#include "runtime/ability/AbilitySystem.hpp"
#include "runtime/ai/NpcPatrolSystem.hpp"
#include "runtime/objective/ObjectiveOutcome.hpp"
#include "runtime/replay/StateHash.hpp"
#include "runtime/session/ScenarioSeedConversion.hpp"

// branch-gate-relocation: BG-1241 from=src/runtime/session/Session.cpp

namespace iggy3d {

namespace {

Result<Session> createFailure(std::string code, std::string message) {
  Result<Session> result;
  result.status = ResultStatus::Error;
  result.error = session_detail::error(std::move(code), std::move(message));
  return result;
}

SessionIdentity createIdentity(std::string packageId, const FixtureScenarioSeed& seed) {
  SessionIdentity identity;
  identity.packageId = std::move(packageId);
  identity.scenarioId = seed.scenarioId;
  identity.sessionSeed = 0;
  identity.schemaVersion = 1;
  return identity;
}

StatusResult createWorld(const FixtureScenarioSeed& seed, WorldState& world) {
  EntityId nextId{1};
  for (const ScenarioEntitySeed& entitySeed : seed.entities) {
    const Result<EntityState> converted = entityFromScenario(entitySeed, nextId);
    if (converted.status != ResultStatus::Ok) {
      return {ResultStatus::Error, converted.error};
    }
    const WorldEntityResult result = world.seedEntity(converted.value);
    if (result.status != WorldStatus::Ok) {
      return session_detail::statusError("session.world_seed_insert_failed",
                                         "failed to insert seeded world entity");
    }
    nextId = nextEntityId(nextId);
  }
  return session_detail::statusOk();
}

StatusResult createPlayers(const FixtureScenarioSeed& seed,
                           const WorldState& world,
                           PlayerRoster& players) {
  for (const ScenarioPlayerSeed& playerSeed : seed.players) {
    const EntityState* actor = world.findByStableName(playerSeed.actorStableName);
    if (actor == nullptr) {
      return session_detail::statusError("session.player_seed_missing_actor",
                                         "failed to resolve player actor stable name");
    }
    const Result<PlayerSlotKind> kind = playerSlotKindFromScenario(playerSeed.kind);
    if (kind.status != ResultStatus::Ok) {
      return {ResultStatus::Error, kind.error};
    }
    PlayerSlot slot;
    slot.id = playerSeed.slot;
    slot.kind = kind.value;
    slot.actor = actor->id;
    slot.stableName = "player" + std::to_string(playerSeed.slot);
    if (players.addSlot(std::move(slot)).status != PlayerRosterStatus::Ok) {
      return session_detail::statusError("session.player_seed_add_failed",
                                         "failed to add player slot");
    }
  }
  if (!players.slotControlsActor(0, EntityId{1})) {
    return session_detail::statusError("session.player_seed_control_failed",
                                       "failed to assign player control slot");
  }
  return session_detail::statusOk();
}

Result<ObjectiveState> createObjectives(const FixtureScenarioSeed& seed) {
  Result<ObjectiveState> result;
  ObjectiveState objectives;
  for (const ScenarioObjectiveSeed& objectiveSeed : seed.objectives) {
    const Result<ObjectiveRecord> objective = objectiveFromScenario(objectiveSeed);
    if (objective.status != ResultStatus::Ok) {
      result.error = objective.error;
      return result;
    }
    objectives.objectives.push_back(std::move(objective.value));
  }
  result.status = ResultStatus::Ok;
  result.value = std::move(objectives);
  return result;
}

InventoryState createInventory(const FixtureScenarioSeed& seed) {
  InventoryState inventory;
  for (const ScenarioPlayerSeed& playerSeed : seed.players) {
    PlayerInventory playerInventory;
    playerInventory.playerSlot = playerSeed.slot;
    inventory.players.push_back(std::move(playerInventory));
  }
  return inventory;
}

StatusResult createCombat(const FixtureScenarioSeed& seed,
                          const WorldState& world,
                          CombatState& combat) {
  for (const ScenarioEntitySeed& entitySeed : seed.entities) {
    if (!entitySeed.combatantEnabled) {
      continue;
    }
    const EntityState* entity = world.findByStableName(entitySeed.stableName);
    if (entity == nullptr) {
      return session_detail::statusError("session.combat_seed_missing_entity",
                                         "failed to resolve combatant entity");
    }
    for (const CombatantState& existing : combat.combatants) {
      if (existing.entity == entity->id) {
        return session_detail::statusError("session.combat_seed_duplicate_entity",
                                           "duplicate combatant entity");
      }
    }
    const Result<std::optional<CombatantState>> converted =
        combatantFromScenario(&entitySeed.combatant, entity->id);
    if (converted.status != ResultStatus::Ok || !converted.value.has_value()) {
      return {ResultStatus::Error, converted.error};
    }
    const CombatantState& combatant = *converted.value;
    if (combatant.maxHitPoints <= 0 || combatant.hitPoints <= 0 ||
        combatant.hitPoints > combatant.maxHitPoints || combatant.defeated) {
      return session_detail::statusError("session.combat_seed_invalid_entity",
                                         "invalid combatant seed");
    }
    combat.combatants.push_back(*converted.value);
  }
  return session_detail::statusOk();
}

Vec3 facingDirectionFromDegrees(float degrees);

StatusResult createAiActors(const FixtureScenarioSeed& seed,
                            const WorldState& world,
                            AiState& ai) {
  std::vector<EntityId> seededActors;
  for (const ScenarioAiActorSeed& aiSeed : seed.aiActors) {
    const Result<PatrolMode> patrolMode = patrolModeFromScenario(aiSeed.patrolMode);
    if (patrolMode.status != ResultStatus::Ok) {
      return {ResultStatus::Error, patrolMode.error};
    }
    const EntityState* actor = world.findByStableName(aiSeed.actorStableName);
    if (actor == nullptr) {
      return session_detail::statusError("session.ai_seed_missing_actor",
                                         "failed to resolve ai actor stable name");
    }
    if (actor->kind != EntityKind::Npc) {
      return session_detail::statusError("session.ai_seed_non_npc_actor",
                                         "ai actor seed must reference an npc");
    }
    if (std::find(seededActors.begin(), seededActors.end(), actor->id) !=
        seededActors.end()) {
      return session_detail::statusError("session.ai_seed_duplicate_actor",
                                         "duplicate ai actor seed");
    }

    AiActorState actorState;
    actorState.actor = actor->id;
    actorState.behaviorProfileId = aiSeed.behaviorProfileId;
    actorState.facingDirection =
        aiSeed.hasFacing
            ? facingDirectionFromDegrees(aiSeed.facingDegrees)
            : session_detail::initialNpcFacing(world, actor->transform.position);
    // Optional authored patrol route (slice 6). Empty = no patrol; a non-empty route
    // must be finite (fail-closed, same pattern as the other ai-seed guards).
    if (!aiSeed.patrolWaypoints.empty()) {
      if (!isValidPatrolRoute(aiSeed.patrolWaypoints)) {
        return session_detail::statusError("session.ai_seed_invalid_patrol_route",
                                           "ai actor patrol route has a non-finite waypoint");
      }
      actorState.patrolWaypoints = aiSeed.patrolWaypoints;
      actorState.patrolMode = patrolMode.value;
    }
    ai.actors.push_back(std::move(actorState));
    seededActors.push_back(actor->id);
  }

  std::vector<EntityId> guardSeededActors;
  for (const ScenarioAiGuardAnchorSeed& guardSeed : seed.aiGuardAnchors) {
    const EntityState* actor = world.findByStableName(guardSeed.actorStableName);
    if (actor == nullptr) {
      return session_detail::statusError("session.guard_seed_missing_actor",
                                         "failed to resolve guard actor stable name");
    }
    if (actor->kind != EntityKind::Npc) {
      return session_detail::statusError("session.guard_seed_non_npc_actor",
                                         "guard actor seed must reference an npc");
    }
    if (std::find(guardSeededActors.begin(), guardSeededActors.end(), actor->id) !=
        guardSeededActors.end()) {
      return session_detail::statusError("session.guard_seed_duplicate_actor",
                                         "duplicate guard actor seed");
    }

    const EntityState* anchor = world.findByStableName(guardSeed.anchorStableName);
    if (anchor == nullptr) {
      return session_detail::statusError("session.guard_seed_missing_anchor",
                                         "failed to resolve guard anchor stable name");
    }
    if (anchor->kind != EntityKind::Marker) {
      return session_detail::statusError("session.guard_seed_non_marker_anchor",
                                         "guard anchor seed must reference a marker");
    }
    if (!std::isfinite(guardSeed.leashRadiusMeters) ||
        !std::isfinite(guardSeed.returnRadiusMeters) ||
        !std::isfinite(guardSeed.homeToleranceMeters) ||
        guardSeed.leashRadiusMeters <= 0.0F ||
        guardSeed.returnRadiusMeters <= 0.0F ||
        guardSeed.homeToleranceMeters < 0.0F ||
        guardSeed.returnRadiusMeters > guardSeed.leashRadiusMeters) {
      return session_detail::statusError("session.guard_seed_invalid_distances",
                                         "invalid guard anchor distances");
    }

    AiActorState* actorState = nullptr;
    for (AiActorState& existing : ai.actors) {
      if (existing.actor == actor->id) {
        actorState = &existing;
        break;
      }
    }
    if (actorState == nullptr) {
      AiActorState inserted;
      inserted.actor = actor->id;
      ai.actors.push_back(std::move(inserted));
      actorState = &ai.actors.back();
    }

    actorState->hasHomePosition = true;
    actorState->homePosition = anchor->transform.position;
    actorState->homeStableName = guardSeed.anchorStableName;
    actorState->leashRadiusMeters = guardSeed.leashRadiusMeters;
    actorState->returnRadiusMeters = guardSeed.returnRadiusMeters;
    actorState->homeToleranceMeters = guardSeed.homeToleranceMeters;
    guardSeededActors.push_back(actor->id);
  }
  return session_detail::statusOk();
}

Result<ClockState> createClock(const SessionCreateRequest& request) {
  Result<ClockState> result;
  const Result<ClockMode> mode = clockModeFromScenario(request.seed.initialClockMode);
  if (mode.status != ResultStatus::Ok) {
    result.error = mode.error;
    return result;
  }
  ClockState clock;
  clock.mode = mode.value;
  clock.previousUnpausedMode = ClockMode::Normal;
  clock.previousUnpausedTimeScale = 1.0F;
  clock.fixedTickRateHz = request.config.fixedTickRateHz;
  clock.timeScale = 1.0F;
  result.status = ResultStatus::Ok;
  result.value = clock;
  return result;
}

Result<CameraState> createCamera(const FixtureScenarioSeed& seed) {
  Result<CameraState> result;
  const Result<CameraMode> realtime = cameraModeFromScenario(seed.defaultRealtimeCamera);
  if (realtime.status != ResultStatus::Ok) {
    result.error = realtime.error;
    return result;
  }
  const Result<CameraMode> tactical = cameraModeFromScenario(seed.defaultTacticalCamera);
  if (tactical.status != ResultStatus::Ok) {
    result.error = tactical.error;
    return result;
  }
  CameraState camera;
  camera.activeMode = realtime.value;
  camera.previousRealtimeMode = realtime.value;
  result.status = ResultStatus::Ok;
  result.value = camera;
  return result;
}

BaselineSnapshot buildBaseline(const SessionState& state) {
  BaselineSnapshot baseline;
  baseline.identity = state.identity;
  baseline.config = state.config;
  baseline.world = state.world;
  baseline.players = state.players;
  baseline.clock = state.clock;
  baseline.camera = state.camera;
  baseline.abilities = state.abilities;
  baseline.inventory = state.inventory;
  baseline.combat = state.combat;
  baseline.ai = state.ai;
  baseline.objectives = state.objectives;
  baseline.baselineHash = computeStateHash(state);
  return baseline;
}

}  // namespace

namespace session_detail {

void clearTransient(SessionState& state) {
  resetAbilityRuntime(state.transient.abilityRuntime);
  state.transient.events.clear();
  state.transient.metrics = {};
  state.transient.pendingExecutionSequences.clear();
  state.transient.lastMovementResultAvailable = false;
  state.transient.lastMovementResult = {};
  state.transient.soundEvents.clear();
  state.transient.cameraInputClearRequested = false;
  state.transient.summaryDirty = true;
  state.transient.stateHashDirty = false;
}

}  // namespace session_detail

namespace {

bool hasValidPlayerZero(const SessionState& state) {
  const PlayerSlot* slot = state.players.findSlot(0);
  return slot != nullptr && state.world.findById(slot->actor) != nullptr;
}

bool lifecyclePairValid(const SessionState& state) {
  if (state.lifecycle == SessionLifecycle::Loading || state.lifecycle == SessionLifecycle::Paused) {
    return false;
  }
  if (state.lifecycle == SessionLifecycle::Complete && state.outcome == SessionOutcome::None) {
    return false;
  }
  if (state.lifecycle == SessionLifecycle::Failed && state.outcome != SessionOutcome::Failed) {
    return false;
  }
  return true;
}

CommandId maxCommandId(const CommandLog& log) {
  CommandId maxId = kInvalidCommandId;
  for (const CommandRecord& record : log.records()) {
    if (record.commandId > maxId) {
      maxId = record.commandId;
    }
  }
  return maxId;
}

bool commandLogValid(const CommandLog& log) {
  CommandLog restored;
  return restored.restoreForLoad(log.records(), log.nextSequence(), log.epoch()).status ==
         CommandLogRestoreStatus::Restored;
}

// Authored spawn facing: yaw degrees about +Y, 0 = +Z, clockwise from above
// (90 = +X), matching the scenario ai_actor facing_degrees convention.
Vec3 facingDirectionFromDegrees(float degrees) {
  constexpr float kDegreesToRadians = 0.01745329252F;
  const float radians = degrees * kDegreesToRadians;
  return Vec3{std::sin(radians), 0.0F, std::cos(radians)};
}

}  // namespace

Session::Session() = default;

Session::Session(SessionState state) : state_(std::move(state)) {}

Result<Session> Session::create(const SessionCreateRequest& request) {
  if (validateRuntimeConfig(request.config) != RuntimeConfigStatus::Ok) {
    return createFailure("session.invalid_config", "invalid runtime config");
  }
  if (request.packageId.empty()) {
    return createFailure("session.invalid_package_id", "invalid package id");
  }
  if (request.seed.scenarioId.empty()) {
    return createFailure("session.invalid_scenario_id", "invalid scenario id");
  }
  if (request.seed.entities.empty()) {
    return createFailure("session.empty_world", "missing entities");
  }

  SessionState state;
  state.identity = createIdentity(request.packageId, request.seed);
  state.config = request.config;
  state.lifecycle = SessionLifecycle::Playing;
  state.outcome = SessionOutcome::None;
  const Result<ClockState> clock = createClock(request);
  if (clock.status != ResultStatus::Ok) {
    return createFailure(clock.error.code, clock.error.message);
  }
  const Result<CameraState> camera = createCamera(request.seed);
  if (camera.status != ResultStatus::Ok) {
    return createFailure(camera.error.code, camera.error.message);
  }
  state.clock = clock.value;
  state.camera = camera.value;
  state.inventory = createInventory(request.seed);
  const Result<ObjectiveState> objectives = createObjectives(request.seed);
  if (objectives.status != ResultStatus::Ok) {
    return createFailure(objectives.error.code, objectives.error.message);
  }
  state.objectives = objectives.value;
  state.outcomeTable = buildObjectiveOutcomeTable();  // A8a: objective->outcome rules as data
  state.nextCommandId = 1;

  const StatusResult world = createWorld(request.seed, state.world);
  if (world.status != ResultStatus::Ok) {
    return createFailure(world.error.code, world.error.message);
  }
  const StatusResult players = createPlayers(request.seed, state.world, state.players);
  if (players.status != ResultStatus::Ok) {
    return createFailure(players.error.code, players.error.message);
  }
  const StatusResult combat = createCombat(request.seed, state.world, state.combat);
  if (combat.status != ResultStatus::Ok) {
    return createFailure(combat.error.code, combat.error.message);
  }
  const StatusResult aiSeed = createAiActors(request.seed, state.world, state.ai);
  if (aiSeed.status != ResultStatus::Ok) {
    return createFailure(aiSeed.error.code, aiSeed.error.message);
  }
  if (state.objectives.objectives.empty()) {
    return createFailure("session.objective_seed_failed", "missing objectives");
  }

  session_detail::clearTransient(state);
  state.currentStateHash = computeStateHash(state);
  state.baseline = buildBaseline(state);

  Result<Session> result;
  result.status = ResultStatus::Ok;
  result.value = Session(std::move(state));
  return result;
}

SessionResetResult Session::resetToBaseline() {
  state_.identity = state_.baseline.identity;
  state_.config = state_.baseline.config;
  state_.world.resetFromBaseline(state_.baseline.world);
  state_.players = state_.baseline.players;
  state_.clock = state_.baseline.clock;
  state_.camera = state_.baseline.camera;
  state_.abilities = state_.baseline.abilities;
  state_.inventory = state_.baseline.inventory;
  state_.combat = state_.baseline.combat;
  state_.ai = state_.baseline.ai;
  state_.objectives = state_.baseline.objectives;
  state_.lifecycle = SessionLifecycle::Playing;
  state_.outcome = SessionOutcome::None;
  state_.commandLog.reset(CommandLogResetPolicy::Clear);
  state_.nextCommandId = 1;
  session_detail::clearTransient(state_);
  state_.currentStateHash = computeStateHash(state_);
  state_.baseline.baselineHash = state_.currentStateHash;
  return {true, state_.baseline.baselineHash, state_.currentStateHash};
}

SessionLoadResult Session::replaceStateFromLoad(SessionState loadedState) {
  const StateHashValue previousHash = state_.currentStateHash;
  SessionLoadResult result;
  result.previousHash = previousHash;

  if (loadedState.identity.packageId.empty() || loadedState.identity.scenarioId.empty() ||
      loadedState.world.empty() || loadedState.players.empty() || !hasValidPlayerZero(loadedState)) {
    result.status = SessionLoadStatus::InvalidCandidateState;
    result.diagnostic = "invalid candidate state";
    return result;
  }
  if (validateRuntimeConfig(loadedState.config) != RuntimeConfigStatus::Ok) {
    result.status = SessionLoadStatus::InvalidCandidateState;
    result.diagnostic = "invalid runtime config";
    return result;
  }
  if (loadedState.nextCommandId == kInvalidCommandId ||
      loadedState.nextCommandId <= maxCommandId(loadedState.commandLog)) {
    result.status = SessionLoadStatus::InvalidCommandIdCursor;
    result.diagnostic = "invalid next command id";
    return result;
  }
  if (!commandLogValid(loadedState.commandLog)) {
    result.status = SessionLoadStatus::InvalidCommandLogState;
    result.diagnostic = "invalid command log";
    return result;
  }
  if (!lifecyclePairValid(loadedState)) {
    result.status = SessionLoadStatus::InvalidLifecycleState;
    result.diagnostic = "invalid lifecycle state";
    return result;
  }

  session_detail::clearTransient(loadedState);
  loadedState.outcomeTable = buildObjectiveOutcomeTable();  // A8a: rebuilt-on-load (transient)
  loadedState.currentStateHash = computeStateHash(loadedState);
  result.loadedHash = loadedState.currentStateHash;
  state_ = std::move(loadedState);
  return result;
}

}  // namespace iggy3d
