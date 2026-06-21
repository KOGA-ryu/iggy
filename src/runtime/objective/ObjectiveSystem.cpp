#include "runtime/objective/ObjectiveSystem.hpp"

#include "runtime/inventory/InventorySystem.hpp"

namespace iggy3d {

namespace {

bool objectiveStatusValid(ObjectiveStatus status) {
  return status == ObjectiveStatus::Inactive || status == ObjectiveStatus::Active ||
         status == ObjectiveStatus::Complete || status == ObjectiveStatus::Failed;
}

bool conditionValid(const ObjectiveCondition& condition) {
  if (condition.kind == ObjectiveConditionKind::None) {
    return true;
  }
  if (condition.kind != ObjectiveConditionKind::PlayerHasItem) {
    return false;
  }
  return isValidPlayerSlotId(condition.playerSlot) && !condition.itemId.empty() &&
         condition.itemCount > 0U;
}

bool hasDuplicateObjectiveIdBefore(const ObjectiveState& objectives, std::size_t index) {
  for (std::size_t prior = 0; prior < index; ++prior) {
    if (objectives.objectives[prior].objectiveId == objectives.objectives[index].objectiveId) {
      return true;
    }
  }
  return false;
}

bool inventoryStructureValid(const InventoryState& inventory) {
  for (const PlayerInventory& playerInventory : inventory.players) {
    if (!isValidPlayerSlotId(playerInventory.playerSlot)) {
      return false;
    }
    for (const InventoryStack& stack : playerInventory.stacks) {
      if (stack.itemId.empty() || stack.count == 0U) {
        return false;
      }
    }
  }
  return true;
}

bool conditionMet(const ObjectiveCondition& condition, const InventoryState& inventory) {
  if (condition.kind == ObjectiveConditionKind::None) {
    return false;
  }
  return hasItem(inventory, condition.playerSlot, condition.itemId, condition.itemCount);
}

}  // namespace

ObjectiveEvaluationStatus validateObjectiveEvaluationContext(
    const ObjectiveState& objectives,
    const InventoryState& inventory) {
  for (std::size_t index = 0; index < objectives.objectives.size(); ++index) {
    const ObjectiveRecord& objective = objectives.objectives[index];
    if (objective.objectiveId.empty() || hasDuplicateObjectiveIdBefore(objectives, index) ||
        !objectiveStatusValid(objective.status) || !conditionValid(objective.condition)) {
      return ObjectiveEvaluationStatus::InvalidObjectiveState;
    }
  }
  if (!inventoryStructureValid(inventory)) {
    return ObjectiveEvaluationStatus::InvalidInventoryState;
  }
  return ObjectiveEvaluationStatus::Evaluated;
}

ObjectiveEvaluationResult evaluateObjectives(
    ObjectiveState& objectives,
    const InventoryState& inventory) {
  ObjectiveEvaluationResult result;
  result.status = validateObjectiveEvaluationContext(objectives, inventory);
  if (result.status != ObjectiveEvaluationStatus::Evaluated) {
    return result;
  }

  for (ObjectiveRecord& objective : objectives.objectives) {
    ++result.evaluatedCount;
    if (objective.status != ObjectiveStatus::Active) {
      continue;
    }
    if (!conditionMet(objective.condition, inventory)) {
      continue;
    }
    objective.status = ObjectiveStatus::Complete;
    ++result.completedThisEvaluation;
    if (objective.objectiveId == "collect_gold_key") {
      result.outcomeChanged = true;
      result.suggestedOutcome = ObjectiveOutcomeSuggestion::DemoComplete;
    }
  }
  return result;
}

bool objectiveComplete(const ObjectiveState& objectives, const std::string& objectiveId) {
  if (objectiveId.empty()) {
    return false;
  }
  for (const ObjectiveRecord& objective : objectives.objectives) {
    if (objective.objectiveId == objectiveId) {
      return objective.status == ObjectiveStatus::Complete;
    }
  }
  return false;
}

}  // namespace iggy3d
