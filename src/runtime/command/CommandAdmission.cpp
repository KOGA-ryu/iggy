#include "runtime/command/CommandAdmission.hpp"

#include <cmath>

#include "runtime/combat/CombatSystem.hpp"
#include "runtime/inventory/InventorySystem.hpp"

namespace iggy3d {

namespace {

bool isPausedAllowedCommand(CommandKind kind) {
  return kind == CommandKind::Resume || kind == CommandKind::StepTacticalTick ||
         kind == CommandKind::Reset || kind == CommandKind::Save || kind == CommandKind::Load ||
         kind == CommandKind::Pause;
}

bool isRetryableSourceKind(CommandKind kind) {
  return kind != CommandKind::None && kind != CommandKind::Retry && kind != CommandKind::Reset &&
         kind != CommandKind::Save && kind != CommandKind::Load;
}

bool requiresConfig(CommandKind kind) {
  return kind == CommandKind::Move || kind == CommandKind::Interact ||
         kind == CommandKind::Attack || kind == CommandKind::Retry;
}

bool requiresCombat(CommandKind kind) {
  return kind == CommandKind::Attack;
}

bool hasNonZeroFiniteDirection(Vec3 direction) {
  return isFinite(direction) && lengthSquared(direction) > 0.000001F;
}

CommandRejectionReason validateContext(
    const CommandAdmissionContext& context,
    CommandKind kind) {
  if (context.world == nullptr || context.players == nullptr || context.clock == nullptr) {
    return CommandRejectionReason::InternalError;
  }
  if (kind == CommandKind::Retry && context.commandLog == nullptr) {
    return CommandRejectionReason::InternalError;
  }
  if (requiresConfig(kind) && context.config == nullptr) {
    return CommandRejectionReason::InternalError;
  }
  if (requiresCombat(kind) && context.combat == nullptr) {
    return CommandRejectionReason::InternalError;
  }
  return CommandRejectionReason::None;
}

CommandRejectionReason validateCommandShape(const CommandRecord& command) {
  if (command.kind == CommandKind::None || command.commandId == kInvalidCommandId ||
      command.admission != CommandAdmissionStatus::Pending) {
    return CommandRejectionReason::InvalidCommand;
  }
  if (requiresActor(command.kind) && !isValid(command.actor)) {
    return CommandRejectionReason::InvalidActor;
  }
  if (command.kind == CommandKind::Retry &&
      command.payload.retrySourceCommandId == kInvalidCommandId) {
    return CommandRejectionReason::RetrySourceMissing;
  }
  if (requiresEntityTarget(command.kind)) {
    if (!command.payload.target.hasEntity || !isValid(command.payload.target.entity)) {
      return CommandRejectionReason::InvalidTarget;
    }
  }
  if (requiresPointTarget(command.kind)) {
    if (!command.payload.target.hasPoint) {
      return CommandRejectionReason::InvalidTargetPoint;
    }
  }
  if (requiresAbilityPayload(command.kind)) {
    if (!isValidCommandAbility(command.payload.ability)) {
      return CommandRejectionReason::InvalidCommand;
    }
    if (!hasNonZeroFiniteDirection(command.payload.abilityDirection)) {
      return CommandRejectionReason::InvalidTargetPoint;
    }
  }
  return CommandRejectionReason::None;
}

CommandRejectionReason validatePlayerSlot(
    const PlayerRoster& players,
    const CommandRecord& command) {
  if (!isValidPlayerSlotId(command.playerSlot)) {
    return CommandRejectionReason::InvalidPlayerSlot;
  }
  const PlayerSlot* slot = players.findSlot(command.playerSlot);
  if (slot == nullptr || !isPlayableSlotKind(slot->kind)) {
    return CommandRejectionReason::InvalidPlayerSlot;
  }
  return CommandRejectionReason::None;
}

CommandRejectionReason validateActorBinding(
    const WorldState& world,
    const PlayerRoster& players,
    const CommandRecord& command) {
  if (!requiresActor(command.kind)) {
    return CommandRejectionReason::None;
  }
  if (!isValid(command.actor)) {
    return CommandRejectionReason::InvalidActor;
  }
  const EntityState* actor = world.findById(command.actor);
  if (actor == nullptr || !actor->active) {
    return CommandRejectionReason::InvalidActor;
  }
  if (!players.slotControlsActor(command.playerSlot, command.actor)) {
    return CommandRejectionReason::ActorNotControlledBySlot;
  }
  return CommandRejectionReason::None;
}

CommandRejectionReason validateClock(
    const ClockState& clock,
    const CommandRecord& command,
    bool allowSessionControlWhilePaused) {
  if (command.kind == CommandKind::StepTacticalTick && clock.mode != ClockMode::Paused) {
    return CommandRejectionReason::StepRequiresPaused;
  }
  if (clock.mode == ClockMode::Paused) {
    if (!allowSessionControlWhilePaused || !isPausedAllowedCommand(command.kind)) {
      return CommandRejectionReason::SessionPaused;
    }
  }
  return CommandRejectionReason::None;
}

CommandRejectionReason validateTargetExistenceAndActivity(
    const WorldState& world,
    const CommandRecord& command) {
  if (!requiresEntityTarget(command.kind)) {
    return CommandRejectionReason::None;
  }
  const EntityId targetId = command.payload.target.entity;
  if (!isValid(targetId)) {
    return CommandRejectionReason::InvalidTarget;
  }
  const EntityState* target = world.findById(targetId);
  if (target == nullptr) {
    return CommandRejectionReason::InvalidTarget;
  }
  if (!target->active) {
    return CommandRejectionReason::TargetInactive;
  }
  return CommandRejectionReason::None;
}

CommandRejectionReason validateTargetability(
    const WorldState& world,
    const CommandRecord& command) {
  if (!requiresEntityTarget(command.kind)) {
    return CommandRejectionReason::None;
  }
  if (command.payload.target.entity == command.actor) {
    return CommandRejectionReason::InvalidTarget;
  }
  const EntityState* target = world.findById(command.payload.target.entity);
  if (target == nullptr || !targetSupportsCommandKind(*target, command.kind)) {
    return CommandRejectionReason::InvalidTarget;
  }
  return CommandRejectionReason::None;
}

CommandRejectionReason validateTargetPoint(const CommandRecord& command) {
  if (!requiresPointTarget(command.kind)) {
    return CommandRejectionReason::None;
  }
  if (!command.payload.target.hasPoint || !isFinite(command.payload.target.point)) {
    return CommandRejectionReason::InvalidTargetPoint;
  }
  return CommandRejectionReason::None;
}

CommandRejectionReason validateReach(
    const CommandAdmissionContext& context,
    const CommandRecord& command) {
  if (command.kind != CommandKind::Interact && command.kind != CommandKind::Attack) {
    return CommandRejectionReason::None;
  }
  if (context.config == nullptr || !std::isfinite(context.config->interactionRangeMeters) ||
      context.config->interactionRangeMeters <= 0.0F) {
    return CommandRejectionReason::InternalError;
  }
  const ReachQueryResult reach = queryReach(
      ReachQueryRequest{context.world, command.actor, command.payload.target.entity, false, {},
                        context.config->interactionRangeMeters, true});
  return rejectionReasonForReach(reach);
}

CommandRejectionReason validateRequiredItem(
    const CommandAdmissionContext& context,
    const CommandRecord& command) {
  if (command.kind != CommandKind::Interact) {
    return CommandRejectionReason::None;
  }
  const EntityState* target = context.world->findById(command.payload.target.entity);
  if (target == nullptr) {
    return CommandRejectionReason::InvalidTarget;
  }
  const InteractionDefinition& interaction = target->interaction;
  if (interaction.requiredItemId.empty() && interaction.requiredItemCount == 0U) {
    return CommandRejectionReason::None;
  }
  if (interaction.requiredItemId.empty() || interaction.requiredItemCount == 0U) {
    return CommandRejectionReason::InternalError;
  }
  if (context.inventory == nullptr) {
    return CommandRejectionReason::InternalError;
  }
  return hasItem(*context.inventory,
                 command.playerSlot,
                 interaction.requiredItemId,
                 interaction.requiredItemCount)
             ? CommandRejectionReason::None
             : CommandRejectionReason::RequiredItemMissing;
}

CommandRejectionReason rejectionReasonForCombat(CombatStatus status) {
  switch (status) {
    case CombatStatus::Succeeded:
      return CommandRejectionReason::None;
    case CombatStatus::InvalidCombatState:
      return CommandRejectionReason::InternalError;
    case CombatStatus::InvalidAttacker:
      return CommandRejectionReason::InvalidActor;
    case CombatStatus::InvalidTarget:
      return CommandRejectionReason::InvalidTarget;
    case CombatStatus::AttackerDefeated:
      return CommandRejectionReason::AttackerDefeated;
    case CombatStatus::TargetDefeated:
      return CommandRejectionReason::TargetDefeated;
    case CombatStatus::FriendlyFireBlocked:
      return CommandRejectionReason::FriendlyFireBlocked;
    case CombatStatus::InvalidDamage:
      return CommandRejectionReason::InvalidDamage;
  }
  return CommandRejectionReason::InternalError;
}

CommandRejectionReason validateKindSpecific(
    const CommandAdmissionContext& context,
    const CommandRecord& command) {
  if (command.kind == CommandKind::Move) {
    if (context.config == nullptr || !std::isfinite(context.config->movementDistanceMeters) ||
        context.config->movementDistanceMeters <= 0.0F) {
      return CommandRejectionReason::InternalError;
    }
    const EntityState* actor = context.world->findById(command.actor);
    if (actor == nullptr) {
      return CommandRejectionReason::InvalidActor;
    }
    const float distance = std::sqrt(distanceSquared(actor->transform.position,
                                                     command.payload.target.point));
    if (!std::isfinite(distance)) {
      return CommandRejectionReason::InvalidTargetPoint;
    }
    if (distance > context.config->movementDistanceMeters) {
      return CommandRejectionReason::MovementTooFar;
    }
  }
  if (command.kind == CommandKind::Attack) {
    if (context.combat == nullptr) {
      return CommandRejectionReason::InternalError;
    }
    const CombatAttackResult combat = previewAttack(
        *context.combat,
        CombatAttackRequest{command.actor, command.payload.target.entity,
                            command.payload.attackDamage, command.commandId});
    return rejectionReasonForCombat(combat.status);
  }
  if (command.kind == CommandKind::Reset) {
    return CommandRejectionReason::ResetUnavailable;
  }
  if (command.kind == CommandKind::Save) {
    return CommandRejectionReason::SaveUnavailable;
  }
  if (command.kind == CommandKind::Load) {
    return CommandRejectionReason::LoadUnavailable;
  }
  return CommandRejectionReason::None;
}

CommandRejectionReason validateExecutableIntent(
    const CommandAdmissionContext& context,
    const CommandRecord& command,
    bool allowSessionControlWhilePaused) {
  if (auto reason = validateCommandShape(command); reason != CommandRejectionReason::None) {
    return reason;
  }
  if (auto reason = validatePlayerSlot(*context.players, command);
      reason != CommandRejectionReason::None) {
    return reason;
  }
  if (auto reason = validateActorBinding(*context.world, *context.players, command);
      reason != CommandRejectionReason::None) {
    return reason;
  }
  if (auto reason = validateClock(*context.clock, command, allowSessionControlWhilePaused);
      reason != CommandRejectionReason::None) {
    return reason;
  }
  if (auto reason = validateTargetExistenceAndActivity(*context.world, command);
      reason != CommandRejectionReason::None) {
    return reason;
  }
  if (auto reason = validateTargetability(*context.world, command);
      reason != CommandRejectionReason::None) {
    return reason;
  }
  if (auto reason = validateTargetPoint(command); reason != CommandRejectionReason::None) {
    return reason;
  }
  if (auto reason = validateReach(context, command); reason != CommandRejectionReason::None) {
    return reason;
  }
  if (auto reason = validateRequiredItem(context, command);
      reason != CommandRejectionReason::None) {
    return reason;
  }
  return validateKindSpecific(context, command);
}

CommandRejectionReason validateRetrySource(
    const CommandAdmissionContext& context,
    const CommandAdmissionRequest& request) {
  const CommandRecord& retry = request.command;
  const CommandLogFindResult found = context.commandLog->findById(retry.payload.retrySourceCommandId);
  if (found.record == nullptr) {
    return CommandRejectionReason::RetrySourceMissing;
  }
  if (found.record->admission != CommandAdmissionStatus::Rejected) {
    return CommandRejectionReason::RetrySourceNotRejected;
  }
  if (!isRetryableSourceKind(found.record->kind)) {
    return CommandRejectionReason::RetryUnsupportedKind;
  }

  CommandRecord effective = *found.record;
  effective.commandId = retry.commandId;
  effective.sequence = kInvalidCommandSequence;
  effective.playerSlot = retry.playerSlot;
  effective.issuedTick = retry.issuedTick;
  effective.scheduledTick = retry.scheduledTick;
  effective.admission = CommandAdmissionStatus::Pending;
  effective.rejection = CommandRejectionReason::None;
  return validateExecutableIntent(context, effective, request.allowSessionControlWhilePaused);
}

}  // namespace

CommandAdmissionResult rejectCommand(CommandRecord command, CommandRejectionReason reason) {
  if (reason == CommandRejectionReason::None) {
    reason = CommandRejectionReason::InternalError;
  }
  command.admission = CommandAdmissionStatus::Rejected;
  command.rejection = reason;
  return {command, reason};
}

CommandAdmissionResult acceptCommand(CommandRecord command) {
  command.admission = CommandAdmissionStatus::Accepted;
  command.rejection = CommandRejectionReason::None;
  return {command, CommandRejectionReason::None};
}

CommandAdmissionResult admitCommand(
    const CommandAdmissionContext& context,
    const CommandAdmissionRequest& request) {
  CommandRecord command = request.command;

  if (auto reason = validateContext(context, command.kind);
      reason != CommandRejectionReason::None) {
    return rejectCommand(command, reason);
  }
  if (auto reason = validateCommandShape(command); reason != CommandRejectionReason::None) {
    return rejectCommand(command, reason);
  }
  if (auto reason = validatePlayerSlot(*context.players, command);
      reason != CommandRejectionReason::None) {
    return rejectCommand(command, reason);
  }
  if (auto reason = validateActorBinding(*context.world, *context.players, command);
      reason != CommandRejectionReason::None) {
    return rejectCommand(command, reason);
  }
  if (auto reason = validateClock(*context.clock, command, request.allowSessionControlWhilePaused);
      reason != CommandRejectionReason::None) {
    return rejectCommand(command, reason);
  }
  if (command.kind == CommandKind::Retry) {
    if (auto reason = validateRetrySource(context, request);
        reason != CommandRejectionReason::None) {
      return rejectCommand(command, reason);
    }
    return acceptCommand(command);
  }
  if (auto reason = validateTargetExistenceAndActivity(*context.world, command);
      reason != CommandRejectionReason::None) {
    return rejectCommand(command, reason);
  }
  if (auto reason = validateTargetability(*context.world, command);
      reason != CommandRejectionReason::None) {
    return rejectCommand(command, reason);
  }
  if (auto reason = validateTargetPoint(command); reason != CommandRejectionReason::None) {
    return rejectCommand(command, reason);
  }
  if (auto reason = validateReach(context, command); reason != CommandRejectionReason::None) {
    return rejectCommand(command, reason);
  }
  if (auto reason = validateRequiredItem(context, command);
      reason != CommandRejectionReason::None) {
    return rejectCommand(command, reason);
  }
  if (auto reason = validateKindSpecific(context, command);
      reason != CommandRejectionReason::None) {
    return rejectCommand(command, reason);
  }
  return acceptCommand(command);
}

}  // namespace iggy3d
