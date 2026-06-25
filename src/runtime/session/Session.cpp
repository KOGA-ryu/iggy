#include "runtime/session/Session.hpp"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

#include "runtime/ability/AbilitySystem.hpp"
#include "runtime/ai/NpcBehaviorProfile.hpp"
#include "runtime/ai/NpcBehaviorSystem.hpp"
#include "runtime/camera/CameraModePolicy.hpp"
#include "runtime/clock/Clock.hpp"
#include "runtime/session/SessionRunner.hpp"
#include "runtime/session/SessionTick.hpp"

namespace iggy3d {

namespace {

ErrorInfo error(std::string code, std::string message) {
  return {std::move(code), std::move(message)};
}

Result<Session> createFailure(std::string code, std::string message) {
  Result<Session> result;
  result.status = ResultStatus::Error;
  result.error = error(std::move(code), std::move(message));
  return result;
}

StatusResult statusError(std::string code, std::string message) {
  return {ResultStatus::Error, error(std::move(code), std::move(message))};
}

StatusResult statusOk() {
  return {ResultStatus::Ok, {}};
}

SessionIdentity createIdentity(std::string packageId, const FixtureScenarioSeed& seed) {
  SessionIdentity identity;
  identity.packageId = std::move(packageId);
  identity.scenarioId = seed.scenarioId;
  identity.sessionSeed = 0;
  identity.schemaVersion = 1;
  return identity;
}

EntityState entityFromSeed(const ScenarioEntitySeed& seed, EntityId id) {
  EntityState entity;
  entity.id = id;
  entity.stableName = seed.stableName;
  entity.kind = seed.kind;
  entity.transform = seed.transform;
  entity.localBounds = seed.localBounds;
  entity.active = seed.active;
  entity.persistent = seed.persistent;
  entity.targeting = seed.targeting;
  entity.interaction = seed.interaction;
  return entity;
}

bool createWorld(const FixtureScenarioSeed& seed, WorldState& world) {
  EntityId nextId{1};
  for (const ScenarioEntitySeed& entitySeed : seed.entities) {
    const WorldEntityResult result = world.seedEntity(entityFromSeed(entitySeed, nextId));
    if (result.status != WorldStatus::Ok) {
      return false;
    }
    nextId = nextEntityId(nextId);
  }
  return true;
}

bool createPlayers(const FixtureScenarioSeed& seed, const WorldState& world, PlayerRoster& players) {
  for (const ScenarioPlayerSeed& playerSeed : seed.players) {
    const EntityState* actor = world.findByStableName(playerSeed.actorStableName);
    if (actor == nullptr) {
      return false;
    }
    PlayerSlot slot;
    slot.id = playerSeed.slot;
    slot.kind = playerSeed.kind;
    slot.actor = actor->id;
    slot.stableName = "player" + std::to_string(playerSeed.slot);
    if (players.addSlot(std::move(slot)).status != PlayerRosterStatus::Ok) {
      return false;
    }
  }
  return players.slotControlsActor(0, EntityId{1});
}

ObjectiveStatus objectiveStatusFromSeed(ObjectiveStatusSeed status) {
  switch (status) {
    case ObjectiveStatusSeed::Active:
      return ObjectiveStatus::Active;
    case ObjectiveStatusSeed::Complete:
      return ObjectiveStatus::Complete;
    case ObjectiveStatusSeed::Failed:
      return ObjectiveStatus::Failed;
  }
  return ObjectiveStatus::Inactive;
}

ObjectiveState createObjectives(const FixtureScenarioSeed& seed) {
  ObjectiveState objectives;
  for (const ScenarioObjectiveSeed& objectiveSeed : seed.objectives) {
    ObjectiveRecord objective;
    objective.objectiveId = objectiveSeed.id;
    objective.status = objectiveStatusFromSeed(objectiveSeed.initialStatus);
    objective.condition.kind = objectiveSeed.condition == "InventoryContains"
                                   ? ObjectiveConditionKind::PlayerHasItem
                                   : ObjectiveConditionKind::None;
    objective.condition.playerSlot = objectiveSeed.playerSlot;
    objective.condition.itemId = objectiveSeed.itemId;
    objective.condition.itemCount = objectiveSeed.itemCount;
    objectives.objectives.push_back(std::move(objective));
  }
  return objectives;
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

bool createCombat(const FixtureScenarioSeed& seed, const WorldState& world, CombatState& combat) {
  for (const ScenarioEntitySeed& entitySeed : seed.entities) {
    if (!entitySeed.combatantEnabled) {
      continue;
    }
    const EntityState* entity = world.findByStableName(entitySeed.stableName);
    if (entity == nullptr) {
      return false;
    }
    for (const CombatantState& existing : combat.combatants) {
      if (existing.entity == entity->id) {
        return false;
      }
    }
    CombatantState combatant = entitySeed.combatant;
    combatant.entity = entity->id;
    combatant.defeated = combatant.hitPoints == 0;
    if (combatant.maxHitPoints <= 0 || combatant.hitPoints <= 0 ||
        combatant.hitPoints > combatant.maxHitPoints || combatant.defeated) {
      return false;
    }
    combat.combatants.push_back(combatant);
  }
  return true;
}

StatusResult createAiActors(const FixtureScenarioSeed& seed,
                            const WorldState& world,
                            AiState& ai) {
  std::vector<EntityId> seededActors;
  for (const ScenarioAiActorSeed& aiSeed : seed.aiActors) {
    const EntityState* actor = world.findByStableName(aiSeed.actorStableName);
    if (actor == nullptr) {
      return statusError("session.ai_seed_missing_actor",
                         "failed to resolve ai actor stable name");
    }
    if (actor->kind != EntityKind::Npc) {
      return statusError("session.ai_seed_non_npc_actor",
                         "ai actor seed must reference an npc");
    }
    if (std::find(seededActors.begin(), seededActors.end(), actor->id) !=
        seededActors.end()) {
      return statusError("session.ai_seed_duplicate_actor",
                         "duplicate ai actor seed");
    }

    AiActorState actorState;
    actorState.actor = actor->id;
    actorState.behaviorProfileId = aiSeed.behaviorProfileId;
    ai.actors.push_back(std::move(actorState));
    seededActors.push_back(actor->id);
  }

  std::vector<EntityId> guardSeededActors;
  for (const ScenarioAiGuardAnchorSeed& guardSeed : seed.aiGuardAnchors) {
    const EntityState* actor = world.findByStableName(guardSeed.actorStableName);
    if (actor == nullptr) {
      return statusError("session.guard_seed_missing_actor",
                         "failed to resolve guard actor stable name");
    }
    if (actor->kind != EntityKind::Npc) {
      return statusError("session.guard_seed_non_npc_actor",
                         "guard actor seed must reference an npc");
    }
    if (std::find(guardSeededActors.begin(), guardSeededActors.end(), actor->id) !=
        guardSeededActors.end()) {
      return statusError("session.guard_seed_duplicate_actor",
                         "duplicate guard actor seed");
    }

    const EntityState* anchor = world.findByStableName(guardSeed.anchorStableName);
    if (anchor == nullptr) {
      return statusError("session.guard_seed_missing_anchor",
                         "failed to resolve guard anchor stable name");
    }
    if (anchor->kind != EntityKind::Marker) {
      return statusError("session.guard_seed_non_marker_anchor",
                         "guard anchor seed must reference a marker");
    }
    if (!std::isfinite(guardSeed.leashRadiusMeters) ||
        !std::isfinite(guardSeed.returnRadiusMeters) ||
        !std::isfinite(guardSeed.homeToleranceMeters) ||
        guardSeed.leashRadiusMeters <= 0.0F ||
        guardSeed.returnRadiusMeters <= 0.0F ||
        guardSeed.homeToleranceMeters < 0.0F ||
        guardSeed.returnRadiusMeters > guardSeed.leashRadiusMeters) {
      return statusError("session.guard_seed_invalid_distances",
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
  return statusOk();
}

ClockState createClock(const SessionCreateRequest& request) {
  ClockState clock;
  clock.mode = request.seed.initialClockMode;
  clock.previousUnpausedMode = ClockMode::Normal;
  clock.previousUnpausedTimeScale = 1.0F;
  clock.fixedTickRateHz = request.config.fixedTickRateHz;
  clock.timeScale = 1.0F;
  return clock;
}

CameraState createCamera(const FixtureScenarioSeed& seed) {
  CameraState camera;
  camera.activeMode = seed.defaultRealtimeCamera;
  camera.previousRealtimeMode = seed.defaultRealtimeCamera;
  return camera;
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

void clearTransient(SessionState& state) {
  resetAbilityRuntime(state.transient.abilityRuntime);
  state.transient.events.clear();
  state.transient.metrics = {};
  state.transient.pendingExecutionSequences.clear();
  state.transient.lastMovementResultAvailable = false;
  state.transient.lastMovementResult = {};
  state.transient.cameraInputClearRequested = false;
  state.transient.summaryDirty = true;
  state.transient.stateHashDirty = false;
}

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

bool queuesForTickExecution(CommandKind kind) {
  return kind == CommandKind::Move || kind == CommandKind::Interact ||
         kind == CommandKind::Inspect || kind == CommandKind::Attack ||
         kind == CommandKind::CastAbility || kind == CommandKind::Wait ||
         kind == CommandKind::Retry;
}

bool sequencePending(const std::vector<CommandSequence>& pending, CommandSequence sequence) {
  for (CommandSequence candidate : pending) {
    if (candidate == sequence) {
      return true;
    }
  }
  return false;
}

bool pendingAbilityCastCommand(const SessionState& state) {
  for (const CommandRecord& record : state.commandLog.records()) {
    if (record.kind == CommandKind::CastAbility &&
        record.admission == CommandAdmissionStatus::Accepted &&
        sequencePending(state.transient.pendingExecutionSequences, record.sequence)) {
      return true;
    }
  }
  return false;
}

AbilityId abilityIdForCommand(CommandAbilityKind ability) {
  switch (ability) {
    case CommandAbilityKind::ArcaneBolt:
      return AbilityId::ArcaneBolt;
    case CommandAbilityKind::None:
      break;
  }
  return AbilityId::None;
}

CommandRejectionReason rejectionForAbilityCastStatus(AbilityCastStatus status) {
  switch (status) {
    case AbilityCastStatus::Accepted:
      return CommandRejectionReason::None;
    case AbilityCastStatus::ProjectileSlotBusy:
      return CommandRejectionReason::AbilitySlotBusy;
    case AbilityCastStatus::OnCooldown:
      return CommandRejectionReason::AbilityOnCooldown;
    case AbilityCastStatus::InsufficientResource:
      return CommandRejectionReason::AbilityInsufficientResource;
    case AbilityCastStatus::InvalidAbility:
    case AbilityCastStatus::InvalidCaster:
    case AbilityCastStatus::InvalidOrigin:
    case AbilityCastStatus::InvalidDirection:
    case AbilityCastStatus::MissingCollisionSurfaces:
      break;
  }
  return CommandRejectionReason::InvalidCommand;
}

AbilityCastRequest abilityCastRequestFromCommand(const SessionState& state,
                                                 const CommandRecord& command,
                                                 const SpatialSurfaceSet& surfaces) {
  AbilityCastRequest request;
  request.ability = abilityIdForCommand(command.payload.ability);
  request.caster = command.actor;
  const EntityState* actor = state.world.findById(command.actor);
  request.originMeters = actor == nullptr
                             ? Vec3{}
                             : actor->transform.position + Vec3{0.0F, 1.65F, 0.0F};
  request.direction = command.payload.abilityDirection;
  request.collisionSurfaces = &surfaces;
  request.sourceCommandId = command.commandId;
  request.currentTick = state.clock.tickIndex;
  return request;
}

CommandAdmissionResult applyAbilityRuntimeAdmission(const SessionState& state,
                                                    const CommandAdmissionResult& admission) {
  if (admission.command.kind != CommandKind::CastAbility ||
      admission.command.admission != CommandAdmissionStatus::Accepted) {
    return admission;
  }
  if (pendingAbilityCastCommand(state)) {
    return rejectCommand(admission.command, CommandRejectionReason::AbilitySlotBusy);
  }

  const SpatialSurfaceSet emptySurfaces;
  const AbilityCastRequest request =
      abilityCastRequestFromCommand(state, admission.command, emptySurfaces);
  const AbilityCastResult inspected =
      inspectAbilityCast(state.abilities, state.transient.abilityRuntime, request);
  if (inspected.accepted) {
    return admission;
  }
  return rejectCommand(admission.command, rejectionForAbilityCastStatus(inspected.status));
}

bool executesImmediately(CommandKind kind) {
  return kind == CommandKind::ToggleTacticalMode || kind == CommandKind::Pause ||
         kind == CommandKind::Resume || kind == CommandKind::StepTacticalTick;
}

void removeExecutedSequences(std::vector<CommandSequence>& pending,
                             const std::vector<CommandSequence>& executed) {
  std::vector<CommandSequence> remaining;
  remaining.reserve(pending.size());
  for (CommandSequence sequence : pending) {
    if (!sequencePending(executed, sequence)) {
      remaining.push_back(sequence);
    }
  }
  pending = std::move(remaining);
}

std::vector<CommandRecord> pendingAcceptedCommands(const SessionState& state) {
  std::vector<CommandRecord> commands;
  for (const CommandRecord& record : state.commandLog.records()) {
    if (sequencePending(state.transient.pendingExecutionSequences, record.sequence) &&
        record.admission == CommandAdmissionStatus::Accepted && queuesForTickExecution(record.kind)) {
      commands.push_back(record);
    }
  }
  return commands;
}

bool tickSucceeded(SessionTickStatus status) {
  return status == SessionTickStatus::Stepped || status == SessionTickStatus::NoWork;
}

void markDirtyAndHash(SessionState& state) {
  state.transient.summaryDirty = true;
  state.transient.stateHashDirty = false;
  state.currentStateHash = computeStateHash(state);
}

bool applyCameraPolicy(SessionState& state) {
  const CameraModePolicyResult camera =
      applyClockModeToCamera(CameraModePolicyRequest{state.camera, state.clock.mode});
  if (camera.status != CameraPolicyStatus::Ok) {
    return false;
  }
  state.camera = camera.camera;
  state.transient.cameraInputClearRequested =
      state.transient.cameraInputClearRequested || camera.camera.inputClearRequested;
  return true;
}

bool applyControlCommand(Session& session, SessionState& state, CommandKind kind) {
  if (kind == CommandKind::ToggleTacticalMode) {
    ClockDecision clock;
    if (state.clock.mode == ClockMode::Normal) {
      clock = enterSlow(state.clock, state.config.slowTimeScale);
    } else if (state.clock.mode == ClockMode::Slow) {
      clock = exitSlow(state.clock);
    } else {
      return false;
    }
    if (clock.status != ClockStatus::Ok) {
      return false;
    }
    state.clock = clock.state;
    return applyCameraPolicy(state);
  }

  if (kind == CommandKind::Pause) {
    const ClockDecision clock = pause(state.clock);
    if (clock.status != ClockStatus::Ok) {
      return false;
    }
    state.clock = clock.state;
    return applyCameraPolicy(state);
  }

  if (kind == CommandKind::Resume) {
    const ClockDecision clock = resume(state.clock);
    if (clock.status != ClockStatus::Ok) {
      return false;
    }
    state.clock = clock.state;
    return applyCameraPolicy(state);
  }

  if (kind == CommandKind::StepTacticalTick) {
    const ClockDecision requested = requestStep(state.clock);
    if (requested.status != ClockStatus::Ok) {
      return false;
    }
    state.clock = requested.state;
    return session.stepOneTick().status == ResultStatus::Ok;
  }

  return false;
}

SessionCommandResult appendCommandThroughAdmission(Session& session,
                                                   SessionState& state,
                                                   const CommandRecord& command) {
  CommandRecord candidate = command;
  candidate.commandId = state.nextCommandId;
  candidate.sequence = kInvalidCommandSequence;
  candidate.issuedTick = state.clock.tickIndex;
  candidate.scheduledTick = state.clock.tickIndex;

  CommandAdmissionResult admission;
  if (state.lifecycle != SessionLifecycle::Playing) {
    admission = rejectCommand(candidate, CommandRejectionReason::SessionNotPlaying);
  } else {
    CommandAdmissionContext context{&state.world, &state.players, &state.clock,
                                    &state.commandLog, &state.config, &state.combat,
                                    &state.inventory};
    admission = applyAbilityRuntimeAdmission(
        state, admitCommand(context, CommandAdmissionRequest{candidate}));
  }

  const CommandLogAppendResult append = state.commandLog.append(admission.command);
  SessionCommandResult result;
  result.command = append.status == CommandLogAppendStatus::Ok ? append.record : admission.command;
  result.admission = admission;
  result.appendStatus = append.status;
  result.appendedToLog = append.status == CommandLogAppendStatus::Ok;
  if (append.status == CommandLogAppendStatus::Ok) {
    state.nextCommandId = candidate.commandId + 1U;
    if (append.record.admission == CommandAdmissionStatus::Accepted) {
      if (executesImmediately(append.record.kind)) {
        result.executedImmediately = applyControlCommand(session, state, append.record.kind);
      } else if (queuesForTickExecution(append.record.kind)) {
        state.transient.pendingExecutionSequences.push_back(append.record.sequence);
      }
    }
    markDirtyAndHash(state);
  }
  return result;
}

bool isActiveNpc(const EntityState& entity) {
  return entity.active && entity.kind == EntityKind::Npc;
}

AiActorState* findAiActorState(AiState& ai, EntityId actor) {
  for (AiActorState& actorState : ai.actors) {
    if (actorState.actor == actor) {
      return &actorState;
    }
  }
  return nullptr;
}

void ensureAiActorsForActiveNpcs(SessionState& state) {
  std::vector<EntityId> activeNpcs;
  for (const EntityState& entity : state.world.entities()) {
    if (isActiveNpc(entity)) {
      activeNpcs.push_back(entity.id);
    }
  }
  std::sort(activeNpcs.begin(), activeNpcs.end());
  for (EntityId actor : activeNpcs) {
    if (findAiActorState(state.ai, actor) == nullptr) {
      AiActorState actorState;
      actorState.actor = actor;
      state.ai.actors.push_back(actorState);
    }
  }
}

bool shouldBuildCommandForDecision(const NpcBehaviorDecision& decision) {
  return decision.status == NpcBehaviorDecisionStatus::Decided ||
         decision.status == NpcBehaviorDecisionStatus::OnCooldown;
}

void applyNpcBehaviorDecision(AiActorState& actorState,
                              const NpcBehaviorDecision& decision) {
  actorState.behavior = decision.behavior;
  actorState.lastIntent = decision.intent;
  actorState.target = decision.target;
  actorState.nextDecisionTick = decision.nextDecisionTick;
  actorState.cooldownTicksRemaining = decision.cooldownTicksRemaining;
}

void enqueueNpcBehaviorCommands(Session& session, SessionState& state) {
  if (state.lifecycle != SessionLifecycle::Playing || state.clock.mode == ClockMode::Paused) {
    return;
  }

  ensureAiActorsForActiveNpcs(state);
  const PlayerSlot* playerZero = state.players.findSlot(0);
  const EntityId target = playerZero == nullptr ? EntityId{} : playerZero->actor;
  const NpcBehaviorProfileCatalog profileCatalog = makeBuiltInNpcBehaviorProfileCatalog();

  std::vector<std::size_t> actorIndexes;
  actorIndexes.reserve(state.ai.actors.size());
  for (std::size_t index = 0; index < state.ai.actors.size(); ++index) {
    const EntityState* actor = state.world.findById(state.ai.actors[index].actor);
    if (actor != nullptr && actor->kind == EntityKind::Npc) {
      actorIndexes.push_back(index);
    }
  }
  std::sort(actorIndexes.begin(), actorIndexes.end(), [&](std::size_t lhs, std::size_t rhs) {
    return state.ai.actors[lhs].actor < state.ai.actors[rhs].actor;
  });

  for (std::size_t index : actorIndexes) {
    AiActorState& actorState = state.ai.actors[index];
    const NpcBehaviorProfileResolveResult resolvedProfile =
        resolveNpcBehaviorProfile({&profileCatalog, actorState.behaviorProfileId});
    if (!resolvedProfile.ok) {
      continue;
    }
    const NpcBehaviorConfig config = resolvedProfile.config;
    const NpcPerceptionResult perception =
        queryNpcPerception(NpcPerceptionRequest{&state.world, &state.combat,
                                                actorState.actor, target, config});
    const NpcBehaviorDecision decision =
        chooseNpcBehaviorIntent(NpcBehaviorDecisionRequest{&actorState, perception, config,
                                                           state.clock.tickIndex});
    applyNpcBehaviorDecision(actorState, decision);
    if (!shouldBuildCommandForDecision(decision)) {
      continue;
    }

    const NpcBehaviorCommandResult command =
        buildNpcBehaviorCommand(NpcBehaviorCommandRequest{decision, perception, config});
    if (command.hasCommand) {
      (void)appendCommandThroughAdmission(session, state, command.command);
    }
  }
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
  state.clock = createClock(request);
  state.camera = createCamera(request.seed);
  state.inventory = createInventory(request.seed);
  state.objectives = createObjectives(request.seed);
  state.nextCommandId = 1;

  if (!createWorld(request.seed, state.world)) {
    return createFailure("session.world_seed_failed", "failed to seed world");
  }
  if (!createPlayers(request.seed, state.world, state.players)) {
    return createFailure("session.player_seed_failed", "failed to seed players");
  }
  if (!createCombat(request.seed, state.world, state.combat)) {
    return createFailure("session.combat_seed_failed", "failed to seed combatants");
  }
  const StatusResult aiSeed = createAiActors(request.seed, state.world, state.ai);
  if (aiSeed.status != ResultStatus::Ok) {
    return createFailure(aiSeed.error.code, aiSeed.error.message);
  }
  if (state.objectives.objectives.empty()) {
    return createFailure("session.objective_seed_failed", "missing objectives");
  }

  clearTransient(state);
  state.currentStateHash = computeStateHash(state);
  state.baseline = buildBaseline(state);

  Result<Session> result;
  result.status = ResultStatus::Ok;
  result.value = Session(std::move(state));
  return result;
}

const SessionState& Session::state() const {
  return state_;
}

SessionState& Session::mutableStateForOwnedSystems() {
  return state_;
}

SessionLifecycle Session::lifecycle() const {
  return state_.lifecycle;
}

SessionOutcome Session::outcome() const {
  return state_.outcome;
}

std::uint64_t Session::stateHash() const {
  return state_.currentStateHash;
}

SessionCommandResult Session::submitCommand(const CommandRecord& command) {
  return appendCommandThroughAdmission(*this, state_, command);
}

StatusResult Session::tick(const SpatialSurfaceSet* collisionSurfaces) {
  enqueueNpcBehaviorCommands(*this, state_);
  std::vector<CommandRecord> commands = pendingAcceptedCommands(state_);
  const SessionTickResult tick =
      runSessionTick(SessionTickInput{&state_, std::move(commands), collisionSurfaces, false});
  removeExecutedSequences(state_.transient.pendingExecutionSequences, tick.executedSequences);

  markDirtyAndHash(state_);

  if (tickSucceeded(tick.status)) {
    return statusOk();
  }
  if (tick.status == SessionTickStatus::BlockedByPausedClock) {
    return statusError("session.tick_paused", "session tick blocked by paused clock");
  }
  if (tick.status == SessionTickStatus::SessionNotPlayable) {
    return statusError("session.tick_not_playable", "session is not playable");
  }
  return statusError("session.tick_invalid_state", "session tick found invalid runtime state");
}

StatusResult Session::stepOneTick(const SpatialSurfaceSet* collisionSurfaces) {
  if (state_.clock.mode != ClockMode::Paused || !state_.clock.stepRequested) {
    return statusError("session.step_requires_paused", "paused step was not requested");
  }

  const ClockDecision consumed = consumeStep(state_.clock);
  if (consumed.status != ClockStatus::Ok || !consumed.consumedStep) {
    return statusError("session.step_invalid_clock", "paused step could not be consumed");
  }
  state_.clock = consumed.state;

  std::vector<CommandRecord> commands = pendingAcceptedCommands(state_);
  if (commands.empty() && !abilityRuntimeHasActiveProjectile(state_.transient.abilityRuntime) &&
      !abilityStateHasPendingRecharge(state_.abilities)) {
    state_.clock = advanceTick(state_.clock);
    ++state_.transient.metrics.ticksRun;
    markDirtyAndHash(state_);
    return statusOk();
  }

  const SessionTickResult tick =
      runSessionTick(SessionTickInput{&state_, std::move(commands), collisionSurfaces, true});
  removeExecutedSequences(state_.transient.pendingExecutionSequences, tick.executedSequences);
  markDirtyAndHash(state_);

  if (tickSucceeded(tick.status)) {
    return statusOk();
  }
  return statusError("session.step_invalid_state", "paused step found invalid runtime state");
}

StatusResult Session::runUntilIdle(std::uint32_t maxTicks,
                                   const SpatialSurfaceSet* collisionSurfaces) {
  SessionRunnerRunResult run =
      runSession(SessionRunnerRunRequest{this, maxTicks, true, true, collisionSurfaces});
  if (run.status == SessionRunnerStatus::Failed) {
    return statusError("session.runner_failed", run.diagnostic);
  }
  if (run.status == SessionRunnerStatus::MaxTicksExceeded) {
    return statusError("session.runner_max_ticks", run.diagnostic);
  }
  return statusOk();
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
  clearTransient(state_);
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

  clearTransient(loadedState);
  loadedState.currentStateHash = computeStateHash(loadedState);
  result.loadedHash = loadedState.currentStateHash;
  state_ = std::move(loadedState);
  return result;
}

SessionFinalizationResult Session::finalizeDemoIfComplete() {
  SessionFinalizationResult result;
  result.previousLifecycle = state_.lifecycle;
  result.lifecycle = state_.lifecycle;
  result.outcome = state_.outcome;
  result.stateHash = state_.currentStateHash;

  if (state_.lifecycle == SessionLifecycle::Complete && state_.outcome == SessionOutcome::DemoComplete) {
    result.status = SessionFinalizationStatus::AlreadyComplete;
    return result;
  }
  if (state_.lifecycle != SessionLifecycle::Playing) {
    result.status = SessionFinalizationStatus::Failed;
    return result;
  }
  if (state_.outcome != SessionOutcome::DemoComplete || state_.clock.mode != ClockMode::Normal ||
      !isRealtimeCameraMode(state_.camera.activeMode) ||
      !state_.transient.pendingExecutionSequences.empty() ||
      abilityRuntimeHasActiveProjectile(state_.transient.abilityRuntime)) {
    result.status = SessionFinalizationStatus::NotReady;
    return result;
  }

  state_.lifecycle = SessionLifecycle::Complete;
  markDirtyAndHash(state_);
  result.status = SessionFinalizationStatus::Completed;
  result.lifecycle = state_.lifecycle;
  result.outcome = state_.outcome;
  result.stateHash = state_.currentStateHash;
  return result;
}

}  // namespace iggy3d
