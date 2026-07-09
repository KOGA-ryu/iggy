#include "runtime/session/SessionTick.hpp"

#include "runtime/ability/AbilitySystem.hpp"
#include "runtime/combat/CombatSystem.hpp"
#include "runtime/interaction/InteractionSystem.hpp"
#include "runtime/movement/MovementSystem.hpp"
#include "runtime/objective/ObjectiveSystem.hpp"

#include <cmath>
#include <string>

namespace iggy3d {

namespace {

constexpr float kCommandAbilityEyeHeightMeters = 1.65F;
constexpr float kFallbackCommandAbilityTickSeconds = 1.0F / 60.0F;

// Ground-plane distance between two points (footsteps are a floor phenomenon; the
// vertical component is irrelevant to how far a footfall carries).
float horizontalDistanceMeters(Vec3 from, Vec3 to) {
  const float dx = to.x - from.x;
  const float dz = to.z - from.z;
  return std::sqrt(dx * dx + dz * dz);
}

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

RuntimeEvent makeAbilityRuntimeEvent(RuntimeEventKind kind,
                                     CommandTick tick,
                                     const AbilityProjectileState& projectile,
                                     EntityId target) {
  RuntimeEvent event;
  event.kind = kind;
  event.tick = tick;
  event.commandId = projectile.sourceCommandId;
  event.actor = projectile.caster;
  event.target = target;
  return event;
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

float commandAbilityTickSeconds(const ClockState& clock) {
  if (clock.fixedTickRateHz == 0U) {
    return kFallbackCommandAbilityTickSeconds;
  }
  return 1.0F / static_cast<float>(clock.fixedTickRateHz);
}

bool executeAbilityCast(SessionState& state,
                        const EffectiveCommandIntent& intent,
                        SessionTickResult& result) {
  const EntityState* actor = state.world.findById(intent.command.actor);
  if (actor == nullptr || !actor->active) {
    return false;
  }

  AbilityCastRequest castRequest;
  castRequest.ability = abilityIdForCommand(intent.command.payload.ability);
  castRequest.caster = intent.command.actor;
  castRequest.originMeters =
      actor->transform.position + Vec3{0.0F, kCommandAbilityEyeHeightMeters, 0.0F};
  castRequest.direction = intent.command.payload.abilityDirection;
  SpatialSurfaceSet emptySurfaces;
  castRequest.collisionSurfaces = &emptySurfaces;
  castRequest.sourceCommandId = intent.sourceCommandId;
  castRequest.currentTick = state.clock.tickIndex;

  const AbilityCastResult cast =
      castAbility(state.abilities, state.transient.abilityRuntime, castRequest);
  if (!cast.accepted) {
    return false;
  }

  ++state.transient.metrics.abilityCasts;
  state.transient.events.push_back(
      makeEvent(RuntimeEventKind::AbilityCast, state.clock.tickIndex, intent));
  ++result.eventsEmitted;
  result.executedSequences.push_back(intent.command.sequence);
  return true;
}

bool tickActiveAbilityRuntime(SessionState& state,
                              const SpatialSurfaceSet* collisionSurfaces,
                              SessionTickResult& result) {
  if (!abilityRuntimeHasActiveProjectile(state.transient.abilityRuntime)) {
    return true;
  }

  SpatialSurfaceSet emptySurfaces;
  const SpatialSurfaceSet* surfaces =
      collisionSurfaces == nullptr ? &emptySurfaces : collisionSurfaces;
  AbilityTickRequest tickRequest;
  tickRequest.collisionSurfaces = surfaces;
  tickRequest.world = &state.world;
  tickRequest.combat = &state.combat;
  tickRequest.deltaSeconds = commandAbilityTickSeconds(state.clock);
  const AbilityTickResult tick = tickAbilityRuntime(state.transient.abilityRuntime, tickRequest);

  if (tick.status == AbilityTickStatus::InvalidInput ||
      tick.status == AbilityTickStatus::MissingCollisionSurfaces) {
    return false;
  }

  if (tick.projectileImpact) {
    ++state.transient.metrics.abilityImpacts;
    state.transient.events.push_back(makeAbilityRuntimeEvent(
        RuntimeEventKind::AbilityImpacted, state.clock.tickIndex,
        state.transient.abilityRuntime.arcaneBolt, tick.hitEntityId));
    ++result.eventsEmitted;
  }

  if (tick.damageApplied) {
    ++result.combatExecuted;
    ++state.transient.metrics.combatExecutions;
    state.transient.events.push_back(makeAbilityRuntimeEvent(
        RuntimeEventKind::CombatAttacked, state.clock.tickIndex,
        state.transient.abilityRuntime.arcaneBolt, tick.hitEntityId));
    ++result.eventsEmitted;
  }
  if (tick.targetDefeated) {
    ++state.transient.metrics.combatDefeats;
    state.transient.events.push_back(makeAbilityRuntimeEvent(
        RuntimeEventKind::CombatantDefeated, state.clock.tickIndex,
        state.transient.abilityRuntime.arcaneBolt, tick.hitEntityId));
    ++result.eventsEmitted;
  }

  return true;
}

bool applyObjectiveOutcome(SessionState& state, SessionTickResult& result) {
  // Objective -> outcome now lives in state.outcomeTable (A8a): evaluate rules IN ORDER; the first
  // rule with a matching complete objective decides. The default table reproduces the two
  // formerly-hardcoded rules exactly (exit_ prefix -> Victory, then collect_gold_key -> DemoComplete),
  // so this is byte-identical to the old path.
  for (const ObjectiveOutcomeRule& rule : state.outcomeTable.rules) {
    bool matched = false;
    for (const ObjectiveRecord& objective : state.objectives.objectives) {
      if (objective.status != ObjectiveStatus::Complete) {
        continue;
      }
      const bool hit = rule.isPrefix ? (objective.objectiveId.rfind(rule.match, 0) == 0)
                                     : (objective.objectiveId == rule.match);
      if (hit) {
        matched = true;
        break;
      }
    }
    if (!matched) {
      continue;
    }
    if (state.outcome != rule.outcome) {
      state.outcome = rule.outcome;
      result.lifecycleChanged = true;
      return true;
    }
    return false;  // the first matching rule decides; outcome already set -> no change
  }
  return false;
}

bool movementBlockConsumesTick(MovementBlockedReason reason) {
  return reason == MovementBlockedReason::BlockedByCollision ||
         reason == MovementBlockedReason::NoWalkableGround ||
         reason == MovementBlockedReason::SlopeRejected;
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
  state.transient.lastMovementResultAvailable = false;
  state.transient.lastMovementResult = {};
  state.transient.soundEvents.clear();  // per-tick sound bus: movement fills, AI loop reads

  if (state.lifecycle != SessionLifecycle::Playing) {
    result.status = SessionTickStatus::SessionNotPlayable;
    return result;
  }
  if (state.clock.mode == ClockMode::Paused && !input.forceStepWhilePaused) {
    result.status = SessionTickStatus::BlockedByPausedClock;
    return result;
  }
  if (input.acceptedCommands.empty() &&
      !abilityRuntimeHasActiveProjectile(state.transient.abilityRuntime) &&
      !abilityStateHasPendingRecharge(state.abilities)) {
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
      MovementSystemContext movementContext{&state.world, &state.config,
                                            input.collisionSurfaces,
                                            input.usePhysicsMovePlanner,
                                            input.precomputedSurfaceBake};
      const MovementMode mode = movementModeForClock(state.clock.mode == ClockMode::Slow);
      const MovementRequest request =
          movementRequestFromAcceptedCommand(intent.command, mode, state.config);
      const MovementResult movement = executeMovement(movementContext, request);
      state.transient.lastMovementResultAvailable = true;
      state.transient.lastMovementResult = movement;
      // EMIT (a1s2, L1): a moving player makes footstep noise. v1 = player footsteps
      // only (guards emit nothing, so no self-hearing). Loudness scales with the
      // actual horizontal displacement this tick; a fully-blocked move (d==0) is
      // silent. Object/combat noise are later A1 sockets. NAMED config, no hardcoded
      // dimensions. Pushed onto the per-tick bus that enqueueNpcBehaviorCommands reads.
      if (intent.command.source == CommandSource::LocalPlayer) {
        const float displacement =
            horizontalDistanceMeters(movement.start, movement.finalPosition);
        if (displacement > 0.0F) {
          SoundEvent footstep;
          footstep.source = intent.command.actor;
          footstep.originMeters = movement.finalPosition;
          footstep.loudnessDb = state.config.footstepBaseLoudnessDb +
                                state.config.footstepLoudnessPerMeterDb * displacement;
          footstep.alertFactor = state.config.footstepAlertFactor;
          footstep.alertMax = state.config.footstepAlertMaxUnits;
          state.transient.soundEvents.push_back(footstep);
        }
      }
      if (movement.blocked != MovementBlockedReason::None) {
        if (movementBlockConsumesTick(movement.blocked)) {
          result.executedSequences.push_back(intent.command.sequence);
          continue;
        }
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

    if (intent.effectiveKind == CommandKind::CastAbility) {
      if (!executeAbilityCast(state, intent, result)) {
        result.status = SessionTickStatus::InvalidState;
        return result;
      }
      continue;
    }

    if (intent.effectiveKind == CommandKind::Inspect || intent.effectiveKind == CommandKind::Wait) {
      result.executedSequences.push_back(intent.command.sequence);
      continue;
    }

    result.status = SessionTickStatus::InvalidState;
    return result;
  }

  if (!tickActiveAbilityRuntime(state, input.collisionSurfaces, result)) {
    result.status = SessionTickStatus::InvalidState;
    return result;
  }

  static_cast<void>(tickAbilityState(state.abilities, state.clock.tickIndex));
  (void)applyObjectiveOutcome(state, result);
  ++state.clock.tickIndex;
  ++state.transient.metrics.ticksRun;
  result.tickAfter = state.clock.tickIndex;
  result.outcome = state.outcome;
  result.status = SessionTickStatus::Stepped;
  return result;
}

}  // namespace iggy3d
