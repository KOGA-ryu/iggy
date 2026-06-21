#pragma once

#include <cstdint>
#include <string>

#include "runtime/inventory/InventoryState.hpp"
#include "runtime/objective/ObjectiveState.hpp"

namespace iggy3d {

enum class ObjectiveEvaluationStatus : std::uint8_t {
  Evaluated,
  InvalidObjectiveState,
  InvalidInventoryState,
};

enum class ObjectiveOutcomeSuggestion : std::uint8_t {
  None,
  DemoComplete,
  Victory,
  Defeat,
};

struct ObjectiveEvaluationResult {
  ObjectiveEvaluationStatus status = ObjectiveEvaluationStatus::Evaluated;
  std::uint32_t evaluatedCount = 0;
  std::uint32_t completedThisEvaluation = 0;
  bool outcomeChanged = false;
  ObjectiveOutcomeSuggestion suggestedOutcome = ObjectiveOutcomeSuggestion::None;
};

ObjectiveEvaluationStatus validateObjectiveEvaluationContext(
    const ObjectiveState& objectives,
    const InventoryState& inventory);

ObjectiveEvaluationResult evaluateObjectives(
    ObjectiveState& objectives,
    const InventoryState& inventory);

bool objectiveComplete(const ObjectiveState& objectives, const std::string& objectiveId);

}  // namespace iggy3d
