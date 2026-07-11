#include "runtime/objective/ObjectiveSystem.hpp"

#include "runtime/inventory/InventorySystem.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

iggy3d::InventoryState inventoryWithGoldKey() {
  iggy3d::InventoryState inventory;
  inventory.players.push_back({0, {}});
  (void)iggy3d::addItem(inventory, {0, "gold_key", 1});
  return inventory;
}

iggy3d::ObjectiveState collectGoldKeyObjective() {
  iggy3d::ObjectiveState objectives;
  iggy3d::ObjectiveRecord objective;
  objective.objectiveId = "collect_gold_key";
  objective.status = iggy3d::ObjectiveStatus::Active;
  objective.condition.kind = iggy3d::ObjectiveConditionKind::PlayerHasItem;
  objective.condition.playerSlot = 0;
  objective.condition.itemId = "gold_key";
  objective.condition.itemCount = 1;
  objectives.objectives.push_back(objective);
  return objectives;
}

bool collectGoldKeyCompletesAndSuggestsDemoComplete() {
  iggy3d::ObjectiveState objectives = collectGoldKeyObjective();
  const iggy3d::InventoryState inventory = inventoryWithGoldKey();
  const iggy3d::ObjectiveEvaluationResult result =
      iggy3d::evaluateObjectives(objectives, inventory);
  return expect(result.status == iggy3d::ObjectiveEvaluationStatus::Evaluated, "evaluated") &&
         expect(result.completedThisEvaluation == 1U, "completed count") &&
         expect(result.suggestedOutcome == iggy3d::ObjectiveOutcomeSuggestion::DemoComplete,
                "demo suggestion") &&
         expect(iggy3d::objectiveComplete(objectives, "collect_gold_key"), "objective complete");
}

bool completedObjectiveIsIdempotent() {
  iggy3d::ObjectiveState objectives = collectGoldKeyObjective();
  const iggy3d::InventoryState inventory = inventoryWithGoldKey();
  (void)iggy3d::evaluateObjectives(objectives, inventory);
  const iggy3d::ObjectiveEvaluationResult second =
      iggy3d::evaluateObjectives(objectives, inventory);
  return expect(second.completedThisEvaluation == 0U, "no second completion") &&
         expect(second.suggestedOutcome == iggy3d::ObjectiveOutcomeSuggestion::None,
                "no second suggestion") &&
         expect(iggy3d::objectiveComplete(objectives, "collect_gold_key"), "still complete");
}

bool invalidStructuresDoNotMutate() {
  iggy3d::ObjectiveState badObjectives = collectGoldKeyObjective();
  badObjectives.objectives[0].objectiveId.clear();
  const iggy3d::InventoryState inventory = inventoryWithGoldKey();
  const iggy3d::ObjectiveEvaluationResult badObjectiveResult =
      iggy3d::evaluateObjectives(badObjectives, inventory);
  bool ok = expect(badObjectiveResult.status ==
                       iggy3d::ObjectiveEvaluationStatus::InvalidObjectiveState,
                   "invalid objective") &&
            expect(badObjectives.objectives[0].status == iggy3d::ObjectiveStatus::Active,
                   "invalid objective no mutation");

  iggy3d::ObjectiveState objectives = collectGoldKeyObjective();
  iggy3d::InventoryState badInventory = inventoryWithGoldKey();
  badInventory.players[0].stacks[0].count = 0;
  const iggy3d::ObjectiveEvaluationResult badInventoryResult =
      iggy3d::evaluateObjectives(objectives, badInventory);
  return ok && expect(badInventoryResult.status ==
                          iggy3d::ObjectiveEvaluationStatus::InvalidInventoryState,
                      "invalid inventory") &&
         expect(objectives.objectives[0].status == iggy3d::ObjectiveStatus::Active,
                "invalid inventory no mutation");
}

}  // namespace

int main() {
  const bool ok = collectGoldKeyCompletesAndSuggestsDemoComplete() &&
                  completedObjectiveIsIdempotent() && invalidStructuresDoNotMutate();
  return ok ? 0 : 1;
}
