#include "runtime/interaction/InteractionSystem.hpp"

namespace iggy3d {

namespace {

InteractionResult baseResult(const InteractionRequest& request, InteractionStatus status) {
  InteractionResult result;
  result.status = status;
  result.actor = request.command.actor;
  result.playerSlot = request.command.playerSlot;
  if (request.command.payload.target.hasEntity) {
    result.target = request.command.payload.target.entity;
  }
  result.sourceCommandId = request.sourceCommandId;
  result.retrySourceCommandId = request.retrySourceCommandId;
  return result;
}

bool commandShapeValid(const CommandRecord& command) {
  return command.admission == CommandAdmissionStatus::Accepted &&
         command.kind == CommandKind::Interact && isValid(command.actor) &&
         command.payload.target.hasEntity && isValid(command.payload.target.entity);
}

void copyInteractionFacts(InteractionResult& result, const InteractionDefinition& interaction) {
  result.kind = interaction.kind;
  result.primaryEffect = interaction.primaryEffect;
  result.itemId = interaction.itemId;
  result.itemCount = interaction.itemCount;
  result.objectiveId = interaction.objectiveId;
  result.deactivateTargetOnSuccess = interaction.deactivateTargetOnSuccess;
}

bool pickupShapeValid(const InteractionDefinition& interaction) {
  return interaction.kind == InteractionKind::Pickup &&
         interaction.primaryEffect == InteractionEffectKind::AddItemToInventory &&
         !interaction.itemId.empty() && interaction.itemCount > 0U;
}

bool emitOnlyShapeValid(const InteractionDefinition& interaction) {
  return interaction.primaryEffect == InteractionEffectKind::EmitEventOnly &&
         (interaction.kind == InteractionKind::OpenDoor ||
          interaction.kind == InteractionKind::Activate ||
          interaction.kind == InteractionKind::ObjectiveTrigger);
}

}  // namespace

InteractionResult executeInteraction(
    InteractionSystemContext& context,
    const InteractionRequest& request) {
  if (!commandShapeValid(request.command)) {
    return baseResult(request, InteractionStatus::InvalidCommand);
  }
  if (context.world == nullptr) {
    return baseResult(request, InteractionStatus::InvalidWorld);
  }
  if (context.inventory == nullptr) {
    return baseResult(request, InteractionStatus::InvalidInventory);
  }
  if (context.objectives == nullptr) {
    return baseResult(request, InteractionStatus::InvalidObjectiveState);
  }

  InteractionResult result = baseResult(request, InteractionStatus::InvalidCommand);
  const EntityState* actor = context.world->findById(request.command.actor);
  if (actor == nullptr || !actor->active) {
    result.status = InteractionStatus::InvalidActor;
    return result;
  }
  const EntityState* target = context.world->findById(request.command.payload.target.entity);
  if (target == nullptr) {
    result.status = InteractionStatus::InvalidTarget;
    return result;
  }
  if (!target->active) {
    result.status = InteractionStatus::TargetInactive;
    return result;
  }

  const InteractionDefinition interaction = target->interaction;
  copyInteractionFacts(result, interaction);

  if (interaction.kind == InteractionKind::Inspect) {
    result.status = InteractionStatus::Succeeded;
    return result;
  }
  if (emitOnlyShapeValid(interaction)) {
    result.status = InteractionStatus::Succeeded;
    return result;
  }
  if (!pickupShapeValid(interaction)) {
    result.status = InteractionStatus::UnsupportedInteraction;
    return result;
  }

  const ObjectiveEvaluationStatus objectiveStatus =
      validateObjectiveEvaluationContext(*context.objectives, *context.inventory);
  if (objectiveStatus != ObjectiveEvaluationStatus::Evaluated) {
    result.status = InteractionStatus::ObjectiveFailed;
    return result;
  }

  const InventoryOperationResult inventory = addItem(
      *context.inventory,
      InventoryItemRequest{request.command.playerSlot, interaction.itemId, interaction.itemCount});
  if (inventory.status != InventoryStatus::Ok) {
    result.status = InteractionStatus::InventoryFailed;
    return result;
  }
  result.inventoryMutated = inventory.mutated;

  if (interaction.deactivateTargetOnSuccess) {
    const WorldMutationResult deactivated =
        context.world->setActive(request.command.payload.target.entity, false);
    if (deactivated.status != WorldStatus::Ok) {
      result.status = InteractionStatus::InvalidWorld;
      return result;
    }
    result.targetDeactivated = true;
  }

  const ObjectiveEvaluationResult objectiveResult =
      evaluateObjectives(*context.objectives, *context.inventory);
  if (objectiveResult.status != ObjectiveEvaluationStatus::Evaluated) {
    result.status = InteractionStatus::ObjectiveFailed;
    return result;
  }
  result.objectiveMutated = objectiveResult.completedThisEvaluation > 0U;
  result.status = InteractionStatus::Succeeded;
  return result;
}

}  // namespace iggy3d
