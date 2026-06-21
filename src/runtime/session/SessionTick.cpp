#include "runtime/session/SessionTick.hpp"

#include "runtime/combat/CombatSystem.hpp"
#include "runtime/interaction/InteractionSystem.hpp"
#include "runtime/movement/MovementSystem.hpp"
#include "runtime/objective/ObjectiveSystem.hpp"

namespace iggy3d {

namespace {

bool isRetryableSourceKind(CommandKind kind) {
  return kind != CommandKind::None && kind != CommandKind::Retry && kind != CommandKind::Reset &&
         kind != CommandKind::Save && kind != CommandKind::Load;
}

bool commandAcceptedForExecution(const CommandRecord& command) {
  return command.admission == CommandAdmissionStatus::Accepted &&
         command.sequence != kInvalidCommandSequence;
}

bool resolveEffectiveIntent(
    const SessionState& state,
    const CommandRecord& command,
    EffectiveCommandIntent& intent) {
  if (!commandAcceptedForExecution(command)) {
    return false;
  }
  if (command.kind != CommandKind::Retry) {
    intent.command = command;
    intent.effectiveKind = command.kind;
    intent.sourceCommandId = command.commandId;
    intent.retrySourceCommandId = kInvalidCommandId;
    return true;
  }

  const CommandLogFindResult found = state.commandLog.findById(command.payload.retrySourceCommandId);
  if (found.record == nullptr || found.record->admission != CommandAdmissionStatus::Rejected ||
      !isRetryableSourceKind(found.record->kind)) {
    return false;
  }

  CommandRecord effective = *found.record;
  effective.commandId = command.commandId;
  effective.sequence = command.sequence;
  effective.playerSlot = command.playerSlot;
  effective.admission = command.admission;
  effective.rejection = command.rejection;

  intent.command = effective;
  intent.effectiveKind = effective.kind;
  intent.sourceCommandId = command.commandId;
  intent.retrySourceCommandId = found.record->commandId;
  return true;
}

RuntimeEvent makeEvent(RuntimeEventKind kind,
                       CommandTick tick,
                       const EffectiveCommandIntent& intent) {
  RuntimeEvent event;
  event.kind = kind;
  event.tick = tick;
  event.commandId = intent.sourceCommandId;
  event.sequence = intent.command.sequence;
  event.playerSlot = intent.command.playerSlot;
  event.actor = intent.command.actor;
  if (intent.command.payload.target.hasEntity) {
    event.target = intent.command.payload.target.entity;
  }
  return event;
}

bool applyObjectiveOutcome(SessionState& state, SessionTickResult& result) {
  if (objectiveComplete(state.objectives, "collect_gold_key") &&
      state.outcome != SessionOutcome::DemoComplete) {
    state.outcome = SessionOutcome::DemoComplete;
    result.lifecycleChanged = true;
    return true;
  }
  return false;
}

}  // namespace

SessionTickResult runSessionTick(const SessionTickInput& input) {
  SessionTickResult result;
  if (input.state == nullptr) {
    return result;
  }

  SessionState& state = *input.state;
  result.tickBefore = state.clock.tickIndex;
  result.tickAfter = state.clock.tickIndex;
  result.outcome = state.outcome;

  if (state.lifecycle != SessionLifecycle::Playing) {
    result.status = SessionTickStatus::SessionNotPlayable;
    return result;
  }
  if (state.clock.mode == ClockMode::Paused && !input.forceStepWhilePaused) {
    result.status = SessionTickStatus::BlockedByPausedClock;
    return result;
  }
  if (input.acceptedCommands.empty()) {
    result.status = SessionTickStatus::NoWork;
    return result;
  }

  std::vector<EffectiveCommandIntent> intents;
  intents.reserve(input.acceptedCommands.size());
  for (const CommandRecord& command : input.acceptedCommands) {
    EffectiveCommandIntent intent;
    if (!resolveEffectiveIntent(state, command, intent)) {
      result.status = SessionTickStatus::InvalidState;
      return result;
    }
    intents.push_back(intent);
  }

  for (const EffectiveCommandIntent& intent : intents) {
    if (intent.effectiveKind == CommandKind::Move) {
      MovementSystemContext movementContext{&state.world, &state.config};
      const MovementMode mode = movementModeForClock(state.clock.mode == ClockMode::Slow);
      const MovementRequest request =
          movementRequestFromAcceptedCommand(intent.command, mode, state.config);
      const MovementResult movement = executeMovement(movementContext, request);
      if (movement.blocked != MovementBlockedReason::None) {
        result.status = SessionTickStatus::InvalidState;
        return result;
      }
      ++result.movementsExecuted;
      ++state.transient.metrics.movementExecutions;
      state.transient.events.push_back(makeEvent(RuntimeEventKind::Moved, state.clock.tickIndex, intent));
      ++result.eventsEmitted;
      result.executedSequences.push_back(intent.command.sequence);
      continue;
    }

    if (intent.effectiveKind == CommandKind::Interact) {
      InteractionSystemContext interactionContext{&state.world, &state.inventory, &state.objectives};
      const InteractionResult interaction = executeInteraction(
          interactionContext,
          InteractionRequest{intent.command, intent.sourceCommandId, intent.retrySourceCommandId});
      if (interaction.status != InteractionStatus::Succeeded) {
        result.status = SessionTickStatus::InvalidState;
        return result;
      }
      ++result.interactionsExecuted;
      ++state.transient.metrics.interactionExecutions;
      state.transient.events.push_back(
          makeEvent(RuntimeEventKind::Interacted, state.clock.tickIndex, intent));
      ++result.eventsEmitted;
      if (interaction.inventoryMutated) {
        ++state.transient.metrics.acquiredItems;
        state.transient.events.push_back(
            makeEvent(RuntimeEventKind::ItemAcquired, state.clock.tickIndex, intent));
        ++result.eventsEmitted;
      }
      if (interaction.objectiveMutated) {
        ++state.transient.metrics.completedObjectives;
        state.transient.events.push_back(
            makeEvent(RuntimeEventKind::ObjectiveCompleted, state.clock.tickIndex, intent));
        ++result.eventsEmitted;
      }
      result.executedSequences.push_back(intent.command.sequence);
      continue;
    }

    if (intent.effectiveKind == CommandKind::Attack) {
      const CombatAttackResult attack =
          applyAttack(state.combat,
                      CombatAttackRequest{intent.command.actor, intent.command.payload.target.entity,
                                          intent.command.payload.attackDamage,
                                          intent.sourceCommandId});
      if (attack.status != CombatStatus::Succeeded) {
        result.status = SessionTickStatus::InvalidState;
        return result;
      }
      ++result.combatExecuted;
      ++state.transient.metrics.combatExecutions;
      state.transient.events.push_back(
          makeEvent(RuntimeEventKind::CombatAttacked, state.clock.tickIndex, intent));
      ++result.eventsEmitted;
      if (attack.targetDefeated) {
        ++state.transient.metrics.combatDefeats;
        state.transient.events.push_back(
            makeEvent(RuntimeEventKind::CombatantDefeated, state.clock.tickIndex, intent));
        ++result.eventsEmitted;
      }
      result.executedSequences.push_back(intent.command.sequence);
      continue;
    }

    if (intent.effectiveKind == CommandKind::Inspect || intent.effectiveKind == CommandKind::Wait) {
      result.executedSequences.push_back(intent.command.sequence);
      continue;
    }

    result.status = SessionTickStatus::InvalidState;
    return result;
  }

  (void)applyObjectiveOutcome(state, result);
  ++state.clock.tickIndex;
  ++state.transient.metrics.ticksRun;
  result.tickAfter = state.clock.tickIndex;
  result.outcome = state.outcome;
  result.status = SessionTickStatus::Stepped;
  return result;
}

}  // namespace iggy3d
