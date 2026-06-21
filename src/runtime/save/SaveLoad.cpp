#include "runtime/save/SaveLoad.hpp"

#include <utility>

namespace iggy3d {

namespace {

SaveStateResult saveFailure(SaveLoadStatus status, std::string diagnostic) {
  SaveStateResult result;
  result.status = status;
  result.diagnostic = std::move(diagnostic);
  return result;
}

LoadStateResult loadFailure(SaveLoadStatus status, StateHashValue previousHash, std::string diagnostic) {
  LoadStateResult result;
  result.status = status;
  result.previousHash = previousHash;
  result.diagnostic = std::move(diagnostic);
  return result;
}

SaveEntityRecord saveEntity(const EntityState& entity) {
  SaveEntityRecord record;
  record.id = entity.id;
  record.stableName = entity.stableName;
  record.kind = entity.kind;
  record.transform = entity.transform;
  record.localBounds = entity.localBounds;
  record.active = entity.active;
  record.persistent = entity.persistent;
  record.targetable = entity.targeting.targetable;
  record.targetActions = entity.targeting.actions;
  record.interactionKind = entity.interaction.kind;
  record.interactionPrimaryEffect = entity.interaction.primaryEffect;
  record.interactionItemId = entity.interaction.itemId;
  record.interactionItemCount = entity.interaction.itemCount;
  record.interactionObjectiveId = entity.interaction.objectiveId;
  record.interactionRepeatable = entity.interaction.repeatable;
  record.interactionDeactivateTargetOnSuccess = entity.interaction.deactivateTargetOnSuccess;
  return record;
}

EntityState loadEntity(const SaveEntityRecord& record) {
  EntityState entity;
  entity.id = record.id;
  entity.stableName = record.stableName;
  entity.kind = record.kind;
  entity.transform = record.transform;
  entity.localBounds = record.localBounds;
  entity.active = record.active;
  entity.persistent = record.persistent;
  entity.targeting.targetable = record.targetable;
  entity.targeting.actions = record.targetActions;
  entity.interaction.kind = record.interactionKind;
  entity.interaction.primaryEffect = record.interactionPrimaryEffect;
  entity.interaction.itemId = record.interactionItemId;
  entity.interaction.itemCount = record.interactionItemCount;
  entity.interaction.objectiveId = record.interactionObjectiveId;
  entity.interaction.repeatable = record.interactionRepeatable;
  entity.interaction.deactivateTargetOnSuccess = record.interactionDeactivateTargetOnSuccess;
  return entity;
}

SaveCommandRecord saveCommand(const CommandRecord& command) {
  SaveCommandRecord record;
  record.commandId = command.commandId;
  record.sequence = command.sequence;
  record.kind = command.kind;
  record.source = command.source;
  record.playerSlot = command.playerSlot;
  record.actor = command.actor;
  record.hasTargetEntity = command.payload.target.hasEntity;
  record.targetEntity = command.payload.target.entity;
  record.hasTargetPoint = command.payload.target.hasPoint;
  record.targetPoint = command.payload.target.point;
  record.retrySourceCommandId = command.payload.retrySourceCommandId;
  record.issuedTick = command.issuedTick;
  record.scheduledTick = command.scheduledTick;
  record.admission = command.admission;
  record.rejection = command.rejection;
  return record;
}

CommandRecord loadCommand(const SaveCommandRecord& record) {
  CommandRecord command;
  command.commandId = record.commandId;
  command.sequence = record.sequence;
  command.kind = record.kind;
  command.source = record.source;
  command.playerSlot = record.playerSlot;
  command.actor = record.actor;
  command.payload.target.hasEntity = record.hasTargetEntity;
  command.payload.target.entity = record.targetEntity;
  command.payload.target.hasPoint = record.hasTargetPoint;
  command.payload.target.point = record.targetPoint;
  command.payload.retrySourceCommandId = record.retrySourceCommandId;
  command.issuedTick = record.issuedTick;
  command.scheduledTick = record.scheduledTick;
  command.admission = record.admission;
  command.rejection = record.rejection;
  return command;
}

bool sourceStateValid(const SessionState& state) {
  if (state.identity.packageId.empty() || state.identity.scenarioId.empty() || state.world.empty() ||
      state.players.empty() || state.nextCommandId == kInvalidCommandId) {
    return false;
  }
  CommandLog restored;
  return restored.restoreForLoad(state.commandLog.records(), state.commandLog.nextSequence(),
                                 state.commandLog.epoch()).status == CommandLogRestoreStatus::Restored;
}

CommandId maxCommandId(const SaveCommandLogSection& commandLog) {
  CommandId maxId = kInvalidCommandId;
  for (const SaveCommandRecord& record : commandLog.records) {
    if (record.commandId > maxId) {
      maxId = record.commandId;
    }
  }
  return maxId;
}

void clearLoadedTransient(SessionState& state) {
  state.clock.stepRequested = false;
  state.camera.inputClearRequested = false;
  state.transient.events.clear();
  state.transient.metrics = {};
  state.transient.pendingExecutionSequences.clear();
  state.transient.cameraInputClearRequested = false;
  state.transient.summaryDirty = true;
  state.transient.stateHashDirty = false;
}

BaselineSnapshot makeLoadedBaseline(const SessionState& state) {
  BaselineSnapshot baseline;
  baseline.identity = state.identity;
  baseline.config = state.config;
  baseline.world = state.world;
  baseline.players = state.players;
  baseline.clock = state.clock;
  baseline.camera = state.camera;
  baseline.inventory = state.inventory;
  baseline.combat = state.combat;
  baseline.ai = state.ai;
  baseline.objectives = state.objectives;
  baseline.baselineHash = computeStateHash(state);
  return baseline;
}

SaveEnvelope envelopeFromState(const SessionState& state) {
  SaveEnvelope envelope;
  const StateHashValue hash = computeStateHash(state);
  envelope.metadata.schemaVersion = kSaveSchemaVersion;
  envelope.metadata.minimumReadableSchemaVersion = kMinimumReadableSaveSchemaVersion;
  envelope.metadata.runtimeSaveVersion = kRuntimeSaveVersion;
  envelope.metadata.packageId = state.identity.packageId;
  envelope.metadata.scenarioId = state.identity.scenarioId;
  envelope.metadata.createdByToolId = "iggy3d.save";
  envelope.metadata.savedStateHash = hash;
  envelope.metadata.savedStateHashHex = formatStateHash(hash);

  envelope.session.lifecycle = state.lifecycle;
  envelope.session.outcome = state.outcome;
  envelope.session.currentTick = state.clock.tickIndex;
  envelope.session.nextCommandId = state.nextCommandId;
  envelope.session.sessionSeed = state.identity.sessionSeed;
  envelope.session.sessionSchemaVersion = state.identity.schemaVersion;
  envelope.session.packageId = state.identity.packageId;
  envelope.session.scenarioId = state.identity.scenarioId;

  envelope.world.nextEntityId = state.world.nextEntityId();
  for (const EntityState& entity : state.world.entities()) {
    envelope.world.entities.push_back(saveEntity(entity));
  }
  for (const PlayerSlot& slot : state.players.slots()) {
    envelope.players.slots.push_back({slot.id, slot.kind, slot.actor, slot.stableName});
  }

  envelope.clock.mode = state.clock.mode;
  envelope.clock.previousUnpausedMode = state.clock.previousUnpausedMode;
  envelope.clock.previousUnpausedTimeScale = state.clock.previousUnpausedTimeScale;
  envelope.clock.tickIndex = state.clock.tickIndex;
  envelope.clock.fixedTickRateHz = state.clock.fixedTickRateHz;
  envelope.clock.timeScale = state.clock.timeScale;

  envelope.camera.activeMode = state.camera.activeMode;
  envelope.camera.previousRealtimeMode = state.camera.previousRealtimeMode;
  envelope.camera.targetEntity = state.camera.target.entity;
  envelope.camera.targetPoint = state.camera.target.point;
  envelope.camera.targetHasPoint = state.camera.target.hasPoint;
  envelope.camera.yawDegrees = state.camera.yawDegrees;
  envelope.camera.pitchDegrees = state.camera.pitchDegrees;
  envelope.camera.orbitDistance = state.camera.orbitDistance;

  envelope.commandLog.nextSequence = state.commandLog.nextSequence();
  envelope.commandLog.epoch = state.commandLog.epoch();
  for (const CommandRecord& command : state.commandLog.records()) {
    envelope.commandLog.records.push_back(saveCommand(command));
  }

  for (const PlayerInventory& inventory : state.inventory.players) {
    SavePlayerInventoryRecord player;
    player.playerSlot = inventory.playerSlot;
    for (const InventoryStack& stack : inventory.stacks) {
      player.stacks.push_back({stack.itemId, stack.count});
    }
    envelope.inventory.players.push_back(std::move(player));
  }
  for (const CombatantState& combatant : state.combat.combatants) {
    envelope.combat.combatants.push_back({combatant.entity, combatant.factionId, combatant.hitPoints,
                                          combatant.maxHitPoints, combatant.defeated});
  }
  for (const AiActorState& actor : state.ai.actors) {
    envelope.ai.actors.push_back(
        {actor.actor, actor.nextDecisionTick, actor.deterministicPolicy, actor.enabled});
  }
  for (const ObjectiveRecord& objective : state.objectives.objectives) {
    envelope.objectives.objectives.push_back({objective.objectiveId, objective.status,
                                              objective.condition.kind,
                                              objective.condition.playerSlot,
                                              objective.condition.itemId,
                                              objective.condition.itemCount});
  }
  return envelope;
}

bool referencesValid(const SessionState& state) {
  for (const PlayerSlot& slot : state.players.slots()) {
    if (!isValidPlayerSlotId(slot.id) || !isValid(slot.actor) ||
        state.world.findById(slot.actor) == nullptr) {
      return false;
    }
  }
  for (const CommandRecord& command : state.commandLog.records()) {
    if (requiresActor(command.kind) && state.world.findById(command.actor) == nullptr) {
      return false;
    }
    if (command.payload.target.hasEntity &&
        state.world.findById(command.payload.target.entity) == nullptr) {
      return false;
    }
  }
  for (const PlayerInventory& inventory : state.inventory.players) {
    if (state.players.findSlot(inventory.playerSlot) == nullptr) {
      return false;
    }
    for (const InventoryStack& stack : inventory.stacks) {
      if (stack.itemId.empty() || stack.count == 0U) {
        return false;
      }
    }
  }
  for (const ObjectiveRecord& objective : state.objectives.objectives) {
    if (objective.objectiveId.empty()) {
      return false;
    }
    if (objective.condition.kind == ObjectiveConditionKind::PlayerHasItem &&
        (state.players.findSlot(objective.condition.playerSlot) == nullptr ||
         objective.condition.itemId.empty() || objective.condition.itemCount == 0U)) {
      return false;
    }
  }
  return true;
}

LoadStateResult buildCandidate(const SaveEnvelope& envelope,
                               StateHashValue previousHash,
                               SessionState& candidate) {
  if (envelope.metadata.packageId != envelope.session.packageId ||
      envelope.metadata.scenarioId != envelope.session.scenarioId ||
      envelope.session.currentTick != envelope.clock.tickIndex ||
      envelope.session.nextCommandId == kInvalidCommandId ||
      envelope.session.nextCommandId <= maxCommandId(envelope.commandLog)) {
    return loadFailure(SaveLoadStatus::InvalidEnvelope, previousHash, "invalid envelope facts");
  }

  candidate.identity.packageId = envelope.session.packageId;
  candidate.identity.scenarioId = envelope.session.scenarioId;
  candidate.identity.sessionSeed = envelope.session.sessionSeed;
  candidate.identity.schemaVersion = envelope.session.sessionSchemaVersion;
  candidate.lifecycle = envelope.session.lifecycle;
  candidate.outcome = envelope.session.outcome;
  candidate.config = makeDefaultRuntimeConfig();
  candidate.nextCommandId = envelope.session.nextCommandId;

  for (const SaveEntityRecord& saved : envelope.world.entities) {
    if (candidate.world.seedEntity(loadEntity(saved)).status != WorldStatus::Ok) {
      return loadFailure(SaveLoadStatus::InvalidReference, previousHash, "invalid world entity");
    }
  }
  for (const SavePlayerSlotRecord& saved : envelope.players.slots) {
    PlayerSlot slot;
    slot.id = saved.slotId;
    slot.kind = saved.kind;
    slot.actor = saved.controlledActor;
    slot.stableName = saved.stableName;
    if (candidate.players.addSlot(slot).status != PlayerRosterStatus::Ok) {
      return loadFailure(SaveLoadStatus::InvalidReference, previousHash, "invalid player slot");
    }
  }

  candidate.clock.mode = envelope.clock.mode;
  candidate.clock.previousUnpausedMode = envelope.clock.previousUnpausedMode;
  candidate.clock.previousUnpausedTimeScale = envelope.clock.previousUnpausedTimeScale;
  candidate.clock.tickIndex = envelope.clock.tickIndex;
  candidate.clock.fixedTickRateHz = envelope.clock.fixedTickRateHz;
  candidate.clock.timeScale = envelope.clock.timeScale;
  candidate.clock.stepRequested = false;

  candidate.camera.activeMode = envelope.camera.activeMode;
  candidate.camera.previousRealtimeMode = envelope.camera.previousRealtimeMode;
  candidate.camera.target.entity = envelope.camera.targetEntity;
  candidate.camera.target.point = envelope.camera.targetPoint;
  candidate.camera.target.hasPoint = envelope.camera.targetHasPoint;
  candidate.camera.yawDegrees = envelope.camera.yawDegrees;
  candidate.camera.pitchDegrees = envelope.camera.pitchDegrees;
  candidate.camera.orbitDistance = envelope.camera.orbitDistance;
  candidate.camera.inputClearRequested = false;

  std::vector<CommandRecord> commands;
  for (const SaveCommandRecord& saved : envelope.commandLog.records) {
    commands.push_back(loadCommand(saved));
  }
  if (candidate.commandLog.restoreForLoad(commands, envelope.commandLog.nextSequence,
                                          envelope.commandLog.epoch).status !=
      CommandLogRestoreStatus::Restored) {
    return loadFailure(SaveLoadStatus::InvalidCommandLog, previousHash, "invalid command log");
  }

  for (const SavePlayerInventoryRecord& saved : envelope.inventory.players) {
    PlayerInventory inventory;
    inventory.playerSlot = saved.playerSlot;
    for (const SaveInventoryStackRecord& stack : saved.stacks) {
      inventory.stacks.push_back({stack.itemId, stack.count});
    }
    candidate.inventory.players.push_back(std::move(inventory));
  }
  for (const SaveCombatantRecord& saved : envelope.combat.combatants) {
    candidate.combat.combatants.push_back(
        {saved.entity, saved.factionId, saved.hitPoints, saved.maxHitPoints, saved.defeated});
  }
  for (const SaveAiActorRecord& saved : envelope.ai.actors) {
    candidate.ai.actors.push_back(
        {saved.actor, saved.nextDecisionTick, saved.deterministicPolicy, saved.enabled});
  }
  for (const SaveObjectiveRecord& saved : envelope.objectives.objectives) {
    ObjectiveRecord objective;
    objective.objectiveId = saved.objectiveId;
    objective.status = saved.status;
    objective.condition.kind = saved.conditionKind;
    objective.condition.playerSlot = saved.conditionPlayerSlot;
    objective.condition.itemId = saved.conditionItemId;
    objective.condition.itemCount = saved.conditionItemCount;
    candidate.objectives.objectives.push_back(std::move(objective));
  }

  if (!referencesValid(candidate)) {
    return loadFailure(SaveLoadStatus::InvalidReference, previousHash, "invalid references");
  }

  clearLoadedTransient(candidate);
  candidate.currentStateHash = computeStateHash(candidate);
  if (candidate.currentStateHash != envelope.metadata.savedStateHash ||
      formatStateHash(candidate.currentStateHash) != envelope.metadata.savedStateHashHex) {
    LoadStateResult result = loadFailure(SaveLoadStatus::HashMismatch, previousHash, "hash mismatch");
    result.loadedHash = candidate.currentStateHash;
    return result;
  }
  candidate.baseline = makeLoadedBaseline(candidate);
  LoadStateResult result;
  result.status = SaveLoadStatus::Ok;
  result.previousHash = previousHash;
  result.loadedHash = candidate.currentStateHash;
  return result;
}

}  // namespace

SaveStateResult saveSessionState(const SessionState& state) {
  if (!sourceStateValid(state)) {
    return saveFailure(SaveLoadStatus::InvalidSourceState, "invalid source state");
  }
  SaveStateResult result;
  result.status = SaveLoadStatus::Ok;
  result.envelope = envelopeFromState(state);
  result.savedStateHash = result.envelope.metadata.savedStateHash;
  return result;
}

SaveStateResult saveSessionStateEncoded(const SessionState& state) {
  SaveStateResult result = saveSessionState(state);
  if (result.status != SaveLoadStatus::Ok) {
    return result;
  }
  const SaveEncodeResult encoded = encodeSaveEnvelope(result.envelope);
  result.codecStatus = encoded.status;
  if (encoded.status != SaveCodecStatus::Ok) {
    result.status = SaveLoadStatus::EncodeFailed;
    result.diagnostic = encoded.diagnostic;
    result.diagnosticSection = encoded.diagnosticSection;
    result.diagnosticKey = encoded.diagnosticKey;
    result.encodedSaveText.clear();
    return result;
  }
  result.encodedSaveText = encoded.encodedText;
  result.savedStateHash = encoded.savedStateHash;
  return result;
}

LoadStateResult loadEnvelopeIntoSession(
    Session& session,
    const SaveEnvelope& envelope,
    const SaveCompatibilityRequest& request) {
  const StateHashValue previousHash = session.stateHash();
  const SaveCompatibilityRequest compatibilityRequest{envelope, request.expectedPackageId,
                                                       request.expectedScenarioId,
                                                       request.currentSchemaVersion,
                                                       request.minimumReadableSchemaVersion,
                                                       request.currentRuntimeSaveVersion};
  const SaveCompatibilityResult compatibility = checkSaveCompatibility(compatibilityRequest);
  if (compatibility.status != SaveCompatibilityStatus::Compatible) {
    LoadStateResult result = loadFailure(SaveLoadStatus::CompatibilityFailed, previousHash,
                                         "compatibility failed");
    result.compatibilityStatus = compatibility.status;
    return result;
  }

  SessionState candidate;
  LoadStateResult candidateResult = buildCandidate(envelope, previousHash, candidate);
  if (candidateResult.status != SaveLoadStatus::Ok) {
    return candidateResult;
  }

  const SessionLoadResult replaced = session.replaceStateFromLoad(std::move(candidate));
  if (replaced.status != SessionLoadStatus::Ok) {
    LoadStateResult result = loadFailure(SaveLoadStatus::ReplacementFailed, previousHash,
                                         replaced.diagnostic);
    result.sessionLoadStatus = replaced.status;
    return result;
  }

  LoadStateResult result;
  result.status = SaveLoadStatus::Ok;
  result.previousHash = previousHash;
  result.loadedHash = replaced.loadedHash;
  return result;
}

LoadStateResult loadEncodedSaveIntoSession(
    Session& session,
    std::string_view encodedSave,
    const SaveCompatibilityRequest& request) {
  const StateHashValue previousHash = session.stateHash();
  const SaveDecodeResult decoded = decodeSaveEnvelope(encodedSave);
  if (decoded.status != SaveCodecStatus::Ok) {
    LoadStateResult result = loadFailure(SaveLoadStatus::DecodeFailed, previousHash,
                                         decoded.diagnostic);
    result.codecStatus = decoded.status;
    return result;
  }
  return loadEnvelopeIntoSession(session, decoded.envelope, request);
}

}  // namespace iggy3d
