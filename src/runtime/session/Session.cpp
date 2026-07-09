#include "runtime/session/Session.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <span>
#include <utility>
#include <vector>

#include "core/math/Vec3.hpp"
#include "runtime/ability/AbilitySystem.hpp"
#include "runtime/ai/NpcAlertSystem.hpp"
#include "runtime/ai/NpcBehaviorProfile.hpp"
#include "runtime/ai/NpcBehaviorSystem.hpp"
#include "runtime/ai/GuardDecision.hpp"
#include "runtime/ai/NpcInvestigateSystem.hpp"
#include "runtime/ai/NpcPatrolSystem.hpp"
#include "runtime/ai/ReasoningRoute.hpp"
#include "runtime/ai/SegmentOcclusion.hpp"
#include "runtime/camera/CameraModePolicy.hpp"
#include "runtime/clock/Clock.hpp"
#include "runtime/physics/PhysicsSpatialSurfaceColliderBake.hpp"
#include "runtime/session/SessionRunner.hpp"
#include "runtime/session/ScenarioSeedConversion.hpp"
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

StatusResult createPlayers(const FixtureScenarioSeed& seed,
                           const WorldState& world,
                           PlayerRoster& players) {
  for (const ScenarioPlayerSeed& playerSeed : seed.players) {
    const EntityState* actor = world.findByStableName(playerSeed.actorStableName);
    if (actor == nullptr) {
      return statusError("session.player_seed_missing_actor",
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
      return statusError("session.player_seed_add_failed",
                         "failed to add player slot");
    }
  }
  if (!players.slotControlsActor(0, EntityId{1})) {
    return statusError("session.player_seed_control_failed",
                       "failed to assign player control slot");
  }
  return statusOk();
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

Vec3 initialNpcFacing(const WorldState& world, Vec3 actorPosition);
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
    actorState.facingDirection =
        aiSeed.hasFacing ? facingDirectionFromDegrees(aiSeed.facingDegrees)
                         : initialNpcFacing(world, actor->transform.position);
    // Optional authored patrol route (slice 6). Empty = no patrol; a non-empty route
    // must be finite (fail-closed, same pattern as the other ai-seed guards).
    if (!aiSeed.patrolWaypoints.empty()) {
      if (!isValidPatrolRoute(aiSeed.patrolWaypoints)) {
        return statusError("session.ai_seed_invalid_patrol_route",
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
      if (const EntityState* actorEntity = state.world.findById(actor)) {
        actorState.facingDirection =
            initialNpcFacing(state.world, actorEntity->transform.position);
      }
      state.ai.actors.push_back(actorState);
    }
  }
}

bool shouldBuildCommandForDecision(const NpcBehaviorDecision& decision) {
  return decision.status == NpcBehaviorDecisionStatus::Decided ||
         decision.status == NpcBehaviorDecisionStatus::OnCooldown;
}

// Local clamp to [0,1] matching NpcAlertSystem.cpp's private clamp01 (NaN -> 0).
float clamp01(float value) {
  if (!(value > 0.0F)) {
    return 0.0F;
  }
  if (value > 1.0F) {
    return 1.0F;
  }
  return value;
}

// A resolved, alive target entity exists (regardless of whether it is currently
// perceived). This is the honest source for the FSM's no-target combat cap: the
// four "target known but not visible right now" statuses still have a live
// target, while defeat/inactive/invalid statuses do not.
bool perceptionHasLiveTarget(NpcPerceptionStatus status) {
  switch (status) {
    case NpcPerceptionStatus::Ready:
    case NpcPerceptionStatus::TargetOutOfRange:
    case NpcPerceptionStatus::TargetOutOfCone:
    case NpcPerceptionStatus::TargetOccluded:
      return true;
    case NpcPerceptionStatus::InvalidWorld:
    case NpcPerceptionStatus::InvalidCombat:
    case NpcPerceptionStatus::InvalidActor:
    case NpcPerceptionStatus::InvalidTarget:
    case NpcPerceptionStatus::InvalidConfig:
    case NpcPerceptionStatus::ActorInactive:
    case NpcPerceptionStatus::TargetInactive:
    case NpcPerceptionStatus::ActorDefeated:
    case NpcPerceptionStatus::TargetDefeated:
      return false;
  }
  return false;
}

// Overlay the graded-alert band onto the fully-alert decision: only the hostile
// combat outcomes (Chasing/Attacking, or OnCooldown) are gated. Below the combat
// band they are downgraded to the alert rung's behavior with a passive Wait
// intent (like a non-engaged NPC). Everything else — leash/return, passive Alert,
// no-target Idle, defeat, disabled, waiting, invalid — passes through unchanged so
// those authoritative outcomes always win over the alert overlay.
NpcBehaviorDecision reconcileAlertBand(const NpcBehaviorDecision& engaged,
                                       const AiActorState& actor,
                                       const AlertProfile& profile) {
  const bool hostileCombat =
      (engaged.status == NpcBehaviorDecisionStatus::Decided &&
       (engaged.behavior == AiBehaviorKind::Chasing ||
        engaged.behavior == AiBehaviorKind::Attacking)) ||
      engaged.status == NpcBehaviorDecisionStatus::OnCooldown;
  if (!hostileCombat) {
    return engaged;
  }

  const std::uint8_t band = alertBandIndex(actor.alertLevel, profile);
  if (band >= 5U) {
    return engaged;  // combat band: keep the range split / cooldown / movement
  }

  // Sub-combat: hold at the alert rung, no attack/move command this tick.
  NpcBehaviorDecision downgraded = engaged;
  downgraded.behavior = alertBehaviorForLevel(actor.alertLevel, profile);
  downgraded.intent = AiIntentKind::Wait;
  downgraded.cooldownTicksRemaining = 0;
  return downgraded;
}

// A resolved perception with a usable actor position (mirrors the NpcBehaviorSystem
// predicate; patrol needs the live position to measure waypoint arrival).
bool perceptionHasActorPosition(const NpcPerceptionResult& perception) {
  return isValid(perception.actor) && isFinite(perception.actorPosition);
}

// Overlay last-known-position investigation onto the reconciled decision (slice 7). Sits in
// precedence BETWEEN combat (kept by reconcileAlertBand at band 5) and patrol (band <=1): when
// the guard is standing/aware (intent==Wait) with memory of a target it can no longer see and
// alert is still in the Searching/Alert band, walk to the remembered spot and look around.
// Gating on intent==Wait leaves combat move/attack, leash ReturnToAnchor, and defeat/disabled
// None untouched. Bands are disjoint from patrol's, so the two overlays never fight.
NpcBehaviorDecision maybeApplyInvestigate(const NpcBehaviorDecision& decision,
                                          AiActorState& actor,
                                          const NpcPerceptionResult& perception,
                                          const AlertProfile& profile,
                                          std::uint64_t tick) {
  static_cast<void>(tick);
  if (decision.intent != AiIntentKind::Wait || !perceptionHasActorPosition(perception)) {
    return decision;
  }

  const std::uint8_t band = alertBandIndex(actor.alertLevel, profile);
  const NpcInvestigateStep step =
      npcStepInvestigate(actor, perception.actorPosition, band, perception.perceived,
                         kPatrolArriveEpsilonMeters, kInvestigateDwellTicks);
  if (!step.active) {
    return decision;
  }

  // Behavior stays derived from the alert level (Searching/Alert); only the intent changes.
  NpcBehaviorDecision investigate = decision;
  investigate.status = NpcBehaviorDecisionStatus::Decided;
  investigate.behavior = alertBehaviorForLevel(actor.alertLevel, profile);
  investigate.cooldownTicksRemaining = 0;
  if (step.dwelling) {
    investigate.intent = AiIntentKind::Wait;  // look around at the spot, no move
  } else {
    investigate.intent = AiIntentKind::Investigate;
    investigate.homePosition = step.destination;
    investigate.returnStopDistanceMeters = kPatrolMoveStopMeters;
  }
  return investigate;
}

// Overlay low-alert patrol onto the reconciled decision (slice 6). Patrol drives
// movement ONLY when the NPC is at rest in the Idle/Observant band; every engaged /
// returning / passive / defeated / cooldown outcome is left untouched (none is an
// Idle/Observant + Wait resting state), so s5 and guard/leash behavior never regress.
NpcBehaviorDecision maybeApplyPatrol(const NpcBehaviorDecision& decision,
                                     AiActorState& actor,
                                     const NpcPerceptionResult& perception,
                                     const AlertProfile& profile) {
  const bool resting =
      decision.intent == AiIntentKind::Wait &&
      (decision.behavior == AiBehaviorKind::Idle ||
       decision.behavior == AiBehaviorKind::Observant);
  if (actor.patrolWaypoints.empty() || !resting ||
      alertBandIndex(actor.alertLevel, profile) > 1U ||
      !perceptionHasActorPosition(perception)) {
    return decision;
  }

  const NpcPatrolStep step =
      npcStepPatrol(actor, perception.actorPosition, kPatrolArriveEpsilonMeters);
  if (!step.active) {
    return decision;
  }

  // Keep the alert-derived behavior + target; switch to a point-move toward the
  // waypoint. status=Decided so shouldBuildCommandForDecision emits the Move (a resting
  // decision is NoTarget, which would emit nothing).
  NpcBehaviorDecision patrol = decision;
  patrol.status = NpcBehaviorDecisionStatus::Decided;
  patrol.intent = AiIntentKind::Patrol;
  patrol.homePosition = step.destination;
  // Rest strictly inside the arrival ring so a cornered approach always registers arrival
  // next tick (see kPatrolMoveStopMeters); arrival precision itself stays at the epsilon.
  patrol.returnStopDistanceMeters = kPatrolMoveStopMeters;
  patrol.cooldownTicksRemaining = 0;
  return patrol;
}

// Scored-search rung (A5 slice 2). A guard still hot (Searching/Alert) whose investigate memory is
// SPENT no longer Waits in place until decay -- it moves between the top-scored reasoning nodes
// (a5s1 chooseSearchNode), steered by the COLD memory sample, riding maybeFollowRoute's flanking.
// Precedence: combat > investigate > SEARCH > patrol, all intent==Wait gated. ZERO new AiIntentKind
// (reuses Investigate -- it IS investigating likely spots; the receipt distinguishes scored-search
// from memory-investigate). NEVER re-scores per tick: choose on entry, re-choose ONLY on arrival
// (alternation via excludedNodeId). Not-hot / has-memory / not-Waiting / EMPTY graph / no candidate
// => decision UNCHANGED (graphless sessions byte-identical). Search state = the actor's TRANSIENT
// fields (never hashed/saved).
NpcBehaviorDecision maybeApplySearch(NpcBehaviorDecision decision, AiActorState& actor, Vec3 guardPos,
                                     std::span<const PhysicsAabbCollider> colliders,
                                     const ReasoningGraph& graph, const AlertProfile& alertProfile,
                                     const NpcPersonalityWeights& weights, std::uint64_t tick) {
  const std::uint8_t band = alertBandIndex(actor.alertLevel, alertProfile);
  if (band < 3U || band > 4U) {
    actor.hasSearchChoice = false;  // cooled out of the search bands -> abandon
    return decision;
  }
  // Trigger keyed to the MEMORY FACT (not the outcome): a DWELLING guard and a VISUALLY-STARING
  // guard both HOLD memory (recording runs before the chain), so hasLastKnownTarget==false excludes
  // both; only a hot guard with SPENT memory searches. intent!=Wait means investigate/patrol/chase
  // already own the tick; empty graph => today's Wait-until-decay.
  if (decision.intent != AiIntentKind::Wait || actor.hasLastKnownTarget || graph.nodes.empty()) {
    return decision;
  }

  const auto nodePos = [&graph](std::uint32_t id) {
    return id < graph.nodes.size() ? graph.nodes[id].positionMeters : Vec3{};
  };
  const auto distanceMeters = [](Vec3 a, Vec3 b) { return std::sqrt(lengthSquared(a - b)); };

  // COLD memory sample: the surviving stale position/tick (flag down) so suspicion still steers the
  // search toward where the target vanished.
  GuardMemorySample memory;
  memory.lastKnownPosition = actor.lastKnownTargetPosition;
  memory.lastKnownTick = actor.lastKnownTargetTick;
  memory.hasMemorySample = actor.hasLastKnownTarget || actor.lastKnownTargetTick != 0;

  // Choose on entry; re-choose ONLY on arrival at the current node, excluding it so the choice
  // ALTERNATES among top nodes (v1 honesty: alternation, not a full circuit).
  bool reChoose = !actor.hasSearchChoice;
  std::optional<std::uint32_t> excluded;
  if (actor.hasSearchChoice &&
      distanceMeters(guardPos, nodePos(actor.searchChosenNodeId)) <= kPatrolArriveEpsilonMeters) {
    reChoose = true;
    excluded = actor.searchChosenNodeId;
  }
  if (reChoose) {
    const GuardDecision chosen = chooseSearchNode(graph, colliders, guardPos, memory, tick,
                                                  actor.actor, weights, GuardDecisionConfig{}, excluded);
    actor.searchLastReceipt = chosen.receipt;
    if (!chosen.nodeId.has_value()) {
      actor.hasSearchChoice = false;
      return decision;  // no candidate -> pass through (today's Wait-until-decay)
    }
    actor.searchChosenNodeId = *chosen.nodeId;
    actor.hasSearchChoice = true;
  }

  // Reuse the Investigate intent toward the chosen node; maybeFollowRoute flanks it if blocked.
  decision.status = NpcBehaviorDecisionStatus::Decided;
  decision.behavior = alertBehaviorForLevel(actor.alertLevel, alertProfile);
  decision.intent = AiIntentKind::Investigate;
  decision.homePosition = nodePos(actor.searchChosenNodeId);
  decision.returnStopDistanceMeters = kPatrolMoveStopMeters;
  decision.cooldownTicksRemaining = 0;
  return decision;
}

// Route-follow POST-STEP (A4 slice 2). After the overlay chain has set a point-move DESTINATION,
// if that destination's straight segment is blocked, steer the guard along a planned graph route
// instead of stalling into the wall. Rewrites ONLY the interim destination + stop distance -- adds
// or reorders NO rung, introduces NO new AiIntentKind. Eligible: Investigate / ReturnToAnchor /
// Patrol (Patrol only actually routes when its waypoint is blocked, which the trigger enforces);
// Chasing/Attacking (band 5) stay DIRECT. EMPTY graph / no path / clear shot => decision untouched
// (today's behavior), so graphless sessions are byte-identical. Route state = the actor's TRANSIENT
// route fields (never hashed/saved). NEVER re-plans per tick: it invalidates on a cheap key
// mismatch and plans only when the final destination is straight-blocked.
NpcBehaviorDecision maybeFollowRoute(NpcBehaviorDecision decision, AiActorState& actor, Vec3 guardPos,
                                     std::span<const PhysicsAabbCollider> colliders,
                                     const ReasoningGraph& graph) {
  const auto clearRoute = [&actor]() {
    actor.hasRoute = false;
    actor.routeNodeIds.clear();
    actor.routeCursor = 0;
  };
  const auto nodePos = [&graph](std::uint32_t id) {
    return id < graph.nodes.size() ? graph.nodes[id].positionMeters : Vec3{};
  };
  const auto distanceMeters = [](Vec3 a, Vec3 b) { return std::sqrt(lengthSquared(a - b)); };

  const bool routable = decision.status == NpcBehaviorDecisionStatus::Decided &&
                        (decision.intent == AiIntentKind::Investigate ||
                         decision.intent == AiIntentKind::ReturnToAnchor ||
                         decision.intent == AiIntentKind::Patrol);
  if (!routable) {
    return decision;  // Chasing/Attacking/Wait/None: never routed; any held route stays dormant.
  }

  const Vec3 finalDestination = decision.homePosition;  // the true target, captured BEFORE rewrite

  // (1) Invalidate a stale route -- cheap equality only, no ray, no Dijkstra. Investigate origins
  // move when fresh noise overwrites the memory; intents flip Return<->Chase<->Return.
  if (actor.hasRoute && (actor.routeIntent != decision.intent ||
                         !nearlyEqual(actor.routePlannedForDestination, finalDestination, 0.05F))) {
    clearRoute();
  }

  // (2) Plan ONLY when the final destination is straight-blocked (one query on the already-baked
  // per-tick occlusion colliders; NO new bake). A clear shot or an empty/no-path result => direct.
  bool justPlanned = false;
  if (!actor.hasRoute) {
    if (!reasoningSegmentBlocked(colliders, guardPos, finalDestination)) {
      return decision;  // clear shot -> today's direct behavior
    }
    const PlannedRoute planned = planRoute(graph, colliders, guardPos, finalDestination, {});
    if (planned.nodeIds.empty()) {
      return decision;  // no path / empty graph / unreachable -> direct (never worse than status quo)
    }
    actor.routeNodeIds = planned.nodeIds;
    actor.routeCursor = 0;
    actor.routeIntent = decision.intent;
    actor.routePlannedForDestination = finalDestination;
    actor.hasRoute = true;
    actor.routeLastPositionMeters = guardPos;
    justPlanned = true;
  }

  // (3) Advance the cursor through every route node already reached.
  const std::uint32_t cursorBefore = actor.routeCursor;
  const std::uint32_t routeSize = static_cast<std::uint32_t>(actor.routeNodeIds.size());
  while (actor.routeCursor < routeSize &&
         distanceMeters(guardPos, nodePos(actor.routeNodeIds[actor.routeCursor])) <=
             kPatrolArriveEpsilonMeters) {
    ++actor.routeCursor;
  }
  if (actor.routeCursor >= routeSize) {
    // Every route node reached -> the wall is flanked; the final leg goes DIRECT to the true target
    // with the intent's OWN stop (already on `decision`). Retire the route.
    clearRoute();
    return decision;
  }

  // (4) No-progress guard (deterministic, per-guard -- NEVER transient.lastMovementResult): if we
  // neither advanced a node nor moved since the previous follow tick, the leg is stuck. A re-plan
  // would reproduce the same blocked node, so DROP to direct rather than loop.
  const bool advancedNode = actor.routeCursor > cursorBefore;
  const bool moved = distanceMeters(guardPos, actor.routeLastPositionMeters) >= kPatrolArriveEpsilonMeters;
  if (!justPlanned && !advancedNode && !moved) {
    clearRoute();
    return decision;
  }
  actor.routeLastPositionMeters = guardPos;

  // (5) Steer to the current interim node with the PATROL stop pair. The intent's own stop (e.g.
  // ReturnToAnchor's larger returnStopDistanceMeters) would freeze the guard AT an interim node; it
  // applies only to the final leg handled in (3).
  decision.homePosition = nodePos(actor.routeNodeIds[actor.routeCursor]);
  decision.returnStopDistanceMeters = kPatrolMoveStopMeters;
  return decision;
}

void applyNpcBehaviorDecision(AiActorState& actorState,
                              const NpcBehaviorDecision& decision) {
  actorState.behavior = decision.behavior;
  actorState.lastIntent = decision.intent;
  actorState.target = decision.target;
  actorState.nextDecisionTick = decision.nextDecisionTick;
  actorState.cooldownTicksRemaining = decision.cooldownTicksRemaining;
}

Vec3 horizontalDirectionOrForward(Vec3 from, Vec3 to) {
  const Vec3 delta{to.x - from.x, 0.0F, to.z - from.z};
  const float lengthSq = lengthSquared(delta);
  if (!std::isfinite(lengthSq) || lengthSq < 1.0e-8F) {
    return Vec3{0.0F, 0.0F, 1.0F};
  }
  const float invLength = 1.0F / std::sqrt(lengthSq);
  return Vec3{delta.x * invLength, 0.0F, delta.z * invLength};
}

// Authored spawn facing: yaw degrees about +Y, 0 = +Z, clockwise from above
// (90 = +X), matching the scenario ai_actor facing_degrees convention.
Vec3 facingDirectionFromDegrees(float degrees) {
  constexpr float kDegreesToRadians = 0.01745329252F;
  const float radians = degrees * kDegreesToRadians;
  return Vec3{std::sin(radians), 0.0F, std::cos(radians)};
}

// Provisional spawn facing until authored orientation exists: point the NPC at
// the first player entity so seeded guards start oriented toward the threat.
Vec3 initialNpcFacing(const WorldState& world, Vec3 actorPosition) {
  for (const EntityState& entity : world.entities()) {
    if (entity.kind == EntityKind::Player) {
      return horizontalDirectionOrForward(actorPosition, entity.transform.position);
    }
  }
  return Vec3{0.0F, 0.0F, 1.0F};
}

void updateNpcFacing(AiActorState& actorState,
                     const NpcBehaviorDecision& decision,
                     const NpcPerceptionResult& perception) {
  switch (decision.intent) {
    case AiIntentKind::MoveTowardTarget:
    case AiIntentKind::AttackTarget:
      actorState.facingDirection = horizontalDirectionOrForward(
          perception.actorPosition, perception.targetPosition);
      break;
    case AiIntentKind::ReturnToAnchor:
    case AiIntentKind::Patrol:
    case AiIntentKind::Investigate:
      // Point-moves face decision.homePosition (anchor / waypoint / last-known sighting).
      actorState.facingDirection = horizontalDirectionOrForward(
          perception.actorPosition, decision.homePosition);
      break;
    case AiIntentKind::None:
    case AiIntentKind::Wait:
      break;  // hold current gaze
  }
}

PhysicsSpatialSurfaceColliderBakeResult bakeSessionTickSurfaceColliders(
    const SpatialSurfaceSet* collisionSurfaces) {
  if (collisionSurfaces == nullptr) {
    return {};
  }

  PhysicsSpatialSurfaceColliderBakeRequest bakeRequest;
  bakeRequest.surfaces = collisionSurfaces;
  return bakePhysicsAabbCollidersFromSpatialSurfaces(bakeRequest);
}

NpcPerceptionResult::Los losFromSegmentOcclusion(SegmentOcclusionVerdict verdict) {
  switch (verdict) {
    case SegmentOcclusionVerdict::Clear:
      return NpcPerceptionResult::Los::Clear;
    case SegmentOcclusionVerdict::Blocked:
      return NpcPerceptionResult::Los::Blocked;
    case SegmentOcclusionVerdict::Unknown:
      return NpcPerceptionResult::Los::Unknown;
  }
  return NpcPerceptionResult::Los::Unknown;
}

AiPerceptionLos aiLosFromPerceptionLos(NpcPerceptionResult::Los los) {
  switch (los) {
    case NpcPerceptionResult::Los::Clear:
      return AiPerceptionLos::Clear;
    case NpcPerceptionResult::Los::Blocked:
      return AiPerceptionLos::Blocked;
    case NpcPerceptionResult::Los::Unknown:
      return AiPerceptionLos::Unknown;
  }
  return AiPerceptionLos::Unknown;
}

// Cast an eye-to-eye segment from actor to target against this tick's baked world colliders.
NpcPerceptionResult::Los actorLineOfSightToTarget(
    const PhysicsSpatialSurfaceColliderBakeResult& bake,
    const EntityState* actor,
    const EntityState* target,
    const NpcBehaviorConfig& config) {
  if (!bake.ok || actor == nullptr || target == nullptr) {
    return NpcPerceptionResult::Los::Unknown;
  }
  Vec3 origin = actor->transform.position;
  origin.y += config.guardEyeHeightMeters;
  Vec3 targetEye = target->transform.position;
  // TODO(P2 follow-up): switch to the sneak eye height when stance is exposed here.
  targetEye.y += config.targetStandEyeHeightMeters;
  return losFromSegmentOcclusion(
      segmentOcclusion(bake.colliders, origin, targetEye, config.occlusionMarginMeters));
}

// Point-to-point occlusion for the sound path (a1s2, L1): same lifted shape as before on raw
// positions, returning APPLY-ONCE whether a wall or unknown bake sits between.
bool soundHasBlockerBetween(const PhysicsSpatialSurfaceColliderBakeResult& bake,
                            Vec3 from,
                            Vec3 to,
                            const NpcBehaviorConfig& config) {
  if (!bake.ok) {
    return true;
  }
  Vec3 origin = from;
  origin.y += config.guardEyeHeightMeters;
  Vec3 destEye = to;
  destEye.y += config.guardEyeHeightMeters;
  const SegmentOcclusionVerdict verdict =
      segmentOcclusion(bake.colliders, origin, destEye, config.occlusionMarginMeters);
  return verdict != SegmentOcclusionVerdict::Clear;
}

void enqueueNpcBehaviorCommands(
    Session& session,
    SessionState& state,
    const PhysicsSpatialSurfaceColliderBakeResult& occlusionBake) {
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
    const AlertProfile alertProfile = resolvedProfile.profile.alertProfile;

    const EntityState* actorEntity = state.world.findById(actorState.actor);
    const EntityState* targetEntity = state.world.findById(target);
    const NpcPerceptionResult::Los targetLos =
        actorLineOfSightToTarget(occlusionBake, actorEntity, targetEntity, config);

    NpcPerceptionRequest perceptionRequest;
    perceptionRequest.world = &state.world;
    perceptionRequest.combat = &state.combat;
    perceptionRequest.actor = actorState.actor;
    perceptionRequest.target = target;
    perceptionRequest.config = config;
    perceptionRequest.actorFacingDirection = actorState.facingDirection;
    perceptionRequest.targetLos = targetLos;
    const NpcPerceptionResult perception = queryNpcPerception(perceptionRequest);

    // Step the graded-alert FSM from the perception already computed. It writes
    // actor.alertLevel (durable) and actor.behavior; the final behavior is set
    // authoritatively by applyNpcBehaviorDecision below, so the intermediate
    // behavior write here is harmless.
    NpcAlertStimulus stimulus;
    stimulus.targetPerceived = perception.perceived;
    stimulus.proximity01 =
        config.perceptionRadiusMeters > 0.0F
            ? clamp01(1.0F - perception.distanceMeters / config.perceptionRadiusMeters)
            : 0.0F;
    stimulus.hasValidTarget = perceptionHasLiveTarget(perception.status);
    stimulus.visualConfirmed = perception.perceived;

    // PERCEIVE (a1s2, L1): resolve THIS tick's sound bus at the guard. Self-hearing
    // skip -- a guard never hears an event it emitted (v1 guards are silent, but the
    // guard is defensively excluded so patrol/idle can't self-alert). blockers[i] is
    // the APPLY-ONCE wall/unknown test between guard and each origin, reusing the tick bake.
    // guardPos comes from actorEntity; if it's missing the guard simply hears nothing.
    if (actorEntity != nullptr && !state.transient.soundEvents.empty()) {
      const Vec3 guardPos = actorEntity->transform.position;
      std::vector<SoundEvent> audibleEvents;
      audibleEvents.reserve(state.transient.soundEvents.size());
      // Parallel blocker buffer. std::vector<bool> is bit-packed and cannot back a
      // std::span<const bool>, so use a plain heap bool[] the span can view.
      auto blockers = std::make_unique<bool[]>(state.transient.soundEvents.size());
      std::size_t heardCount = 0;
      for (const SoundEvent& event : state.transient.soundEvents) {
        if (event.source == actorState.actor) {
          continue;  // never hear yourself
        }
        blockers[heardCount] =
            soundHasBlockerBetween(occlusionBake, guardPos, event.originMeters, config);
        audibleEvents.push_back(event);
        ++heardCount;
      }
      const SoundPerceptionResult snd = resolveLoudestSound(
          audibleEvents, guardPos, resolvedProfile.profile.soundConfig,
          std::span<const bool>(blockers.get(), heardCount));
      stimulus.heard = snd.heard;
      stimulus.audibilityDb = snd.audibilityDb;
      stimulus.alertUnits = snd.alertUnits;
      stimulus.soundInvestigatePos = snd.investigatePos;
    }

    const std::uint8_t alertBandBefore = alertBandIndex(actorState.alertLevel, alertProfile);
    npcStepAlert(actorState, stimulus, alertProfile, state.clock.tickIndex);
    const std::uint8_t alertBandAfter = alertBandIndex(actorState.alertLevel, alertProfile);
    const bool alertBandIncreased = alertBandAfter > alertBandBefore;
    // Remember where the target is while it is actually seen (slice 7). visualConfirmed implies
    // Ready, so perception.targetPosition is the live sighting. MEMORY (a1s2/P5b): if the target
    // was heard but never seen this tick, share the SAME last-known memory but only refresh it
    // for a relocated sound or strict alert-band rise. Visual always wins when both are present.
    if (stimulus.visualConfirmed) {
      npcRecordSighting(actorState, perception.targetPosition, state.clock.tickIndex);
    } else if (stimulus.heard) {
      (void)npcRecordNonvisualInvestigationMemory(
          actorState, stimulus.soundInvestigatePos, state.clock.tickIndex,
          alertBandIncreased);
    }

    const NpcBehaviorDecision engaged =
        chooseNpcBehaviorIntent(NpcBehaviorDecisionRequest{&actorState, perception, config,
                                                           state.clock.tickIndex});
    NpcBehaviorDecision decision =
        reconcileAlertBand(engaged, actorState, alertProfile);
    // Precedence: combat (kept above) > investigate last-known (band 3-4) > SEARCH (a5s2) > patrol.
    decision =
        maybeApplyInvestigate(decision, actorState, perception, alertProfile, state.clock.tickIndex);
    // SEARCH rung (a5s2): a hot guard with SPENT memory checks scored nodes instead of Waiting.
    if (actorEntity != nullptr && occlusionBake.ok) {
      decision = maybeApplySearch(decision, actorState, actorEntity->transform.position,
                                  occlusionBake.colliders, state.reasoningGraph, alertProfile,
                                  resolvedProfile.profile.personalityWeights, state.clock.tickIndex);
    }
    decision = maybeApplyPatrol(decision, actorState, perception, alertProfile);
    // Route-follow POST-STEP (A4 s2): flank blocked destinations via the carried reasoning graph.
    // Rewrites only the interim destination inside the rung above; empty graph -> unchanged.
    if (actorEntity != nullptr && occlusionBake.ok) {
      decision = maybeFollowRoute(decision, actorState, actorEntity->transform.position,
                                  occlusionBake.colliders, state.reasoningGraph);
    }
    applyNpcBehaviorDecision(actorState, decision);
    updateNpcFacing(actorState, decision, perception);
    actorState.lastTargetInRadius = perception.targetInPerceptionRadius;
    actorState.lastTargetInVisionCone = perception.targetInVisionCone;
    actorState.lastTargetHasLineOfSight = perception.hasLineOfSight;
    actorState.lastSightRangeMeters = config.perceptionRadiusMeters;
    actorState.lastHorizontalAngleDeg = perception.horizontalAngleDeg;
    actorState.lastVerticalAngleDeg = perception.verticalAngleDeg;
    actorState.lastInVerticalCone = perception.inVerticalCone;
    actorState.lastPerceived = perception.perceived;
    actorState.lastLos = aiLosFromPerceptionLos(perception.los);
    actorState.lastGuardEyeHeightMeters = config.guardEyeHeightMeters;
    actorState.lastVerticalHalfAngleDegrees = config.verticalHalfAngleDegrees;

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
  state.objectives = createObjectives(request.seed);
  state.outcomeTable = buildObjectiveOutcomeTable();  // A8a: objective->outcome rules as data
  state.nextCommandId = 1;

  if (!createWorld(request.seed, state.world)) {
    return createFailure("session.world_seed_failed", "failed to seed world");
  }
  const StatusResult players = createPlayers(request.seed, state.world, state.players);
  if (players.status != ResultStatus::Ok) {
    return createFailure(players.error.code, players.error.message);
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

void Session::setReasoningGraph(ReasoningGraph graph) {
  // Set-once carry: the session takes ownership of the caller-built graph. Off StateHash/SaveCodec,
  // so this never shifts a receipt or hash (A3 zero-behavior-change contract).
  state_.reasoningGraph = std::move(graph);
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
  return tickWithOptions(SessionTickOptions{collisionSurfaces, false});
}

StatusResult Session::tickWithOptions(const SessionTickOptions& options) {
  // Bake once for this tick. A successful empty bake is clear-capable open
  // space; an absent/failed bake stays Unknown for sense callers.
  const PhysicsSpatialSurfaceColliderBakeResult tickSurfaceBake =
      bakeSessionTickSurfaceColliders(options.collisionSurfaces);
  enqueueNpcBehaviorCommands(*this, state_, tickSurfaceBake);
  std::vector<CommandRecord> commands = pendingAcceptedCommands(state_);
  const SessionTickResult tick =
      runSessionTick(SessionTickInput{&state_,
                                      std::move(commands),
                                      options.collisionSurfaces,
                                      false,
                                      options.usePhysicsMovePlanner,
                                      options.collisionSurfaces == nullptr
                                          ? nullptr
                                          : &tickSurfaceBake});
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
  return stepOneTickWithOptions(SessionTickOptions{collisionSurfaces, false});
}

StatusResult Session::stepOneTickWithOptions(const SessionTickOptions& options) {
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

  const PhysicsSpatialSurfaceColliderBakeResult tickSurfaceBake =
      bakeSessionTickSurfaceColliders(options.collisionSurfaces);
  const SessionTickResult tick =
      runSessionTick(SessionTickInput{&state_,
                                      std::move(commands),
                                      options.collisionSurfaces,
                                      true,
                                      options.usePhysicsMovePlanner,
                                      options.collisionSurfaces == nullptr
                                          ? nullptr
                                          : &tickSurfaceBake});
  removeExecutedSequences(state_.transient.pendingExecutionSequences, tick.executedSequences);
  markDirtyAndHash(state_);

  if (tickSucceeded(tick.status)) {
    return statusOk();
  }
  return statusError("session.step_invalid_state", "paused step found invalid runtime state");
}

StatusResult Session::runUntilIdle(std::uint32_t maxTicks,
                                   const SpatialSurfaceSet* collisionSurfaces) {
  return runUntilIdleWithOptions(maxTicks, SessionTickOptions{collisionSurfaces, false});
}

StatusResult Session::runUntilIdleWithOptions(std::uint32_t maxTicks,
                                              const SessionTickOptions& options) {
  SessionRunnerRunResult run =
      runSession(SessionRunnerRunRequest{this,
                                         maxTicks,
                                         true,
                                         true,
                                         options.collisionSurfaces,
                                         options.usePhysicsMovePlanner});
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
  loadedState.outcomeTable = buildObjectiveOutcomeTable();  // A8a: rebuilt-on-load (transient)
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
