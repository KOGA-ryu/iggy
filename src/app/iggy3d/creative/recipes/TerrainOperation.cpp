#include "app/iggy3d/creative/recipes/TerrainOperation.hpp"

#include "core/hash/StableHash.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace iggy3d::creative {
namespace {

[[nodiscard]] std::uint64_t heightFieldHash(
    const CreativeTerrainHeightField& field) noexcept {
  const CreativeTerrainHeightFieldBounds bounds = field.bounds();
  StableHasher hasher;
  hasher.addI64(bounds.minimum.x);
  hasher.addI64(bounds.minimum.z);
  hasher.addU64(bounds.widthCells);
  hasher.addU64(bounds.depthCells);
  for (const std::uint16_t height : field.heights()) {
    hasher.addU64(height);
  }
  return hasher.value();
}

void setReplayFailure(CreativeTerrainOperationReplayReceipt& receipt,
                      CreativeTerrainOperationReplayStatus status,
                      std::size_t index,
                      CreativeTerrainOperationId operationId,
                      std::string_view reasonCode) noexcept {
  receipt.status = status;
  receipt.failedOperationIndex = index;
  receipt.failedOperationId = operationId;
  receipt.reasonCode = reasonCode;
}

void setMutationStatus(CreativeTerrainOperationMutationReceipt& receipt,
                       CreativeTerrainOperationMutationStatus status,
                       std::string_view reasonCode) noexcept {
  receipt.status = status;
  receipt.reasonCode = reasonCode;
}

[[nodiscard]] bool requestCarriesValidRecipe(
    const CreativeTerrainOperationMutationRequest& request) noexcept {
  return isValidCreativeTerrainGeneratorRecipe(request.generation) &&
         isValidCreativeTerrainCompositionRecipe(request.composition);
}

}  // namespace

bool creativeTerrainHeightFieldsEqual(
    const CreativeTerrainHeightField& lhs,
    const CreativeTerrainHeightField& rhs) noexcept {
  return lhs.validateInvariants() && rhs.validateInvariants() &&
         lhs.bounds() == rhs.bounds() &&
         lhs.heights().size() == rhs.heights().size() &&
         std::equal(lhs.heights().begin(), lhs.heights().end(),
                    rhs.heights().begin(), rhs.heights().end());
}

bool validateCreativeTerrainOperationStack(
    const CreativeTerrainOperationStack& stack) noexcept {
  if (stack.version != kCreativeTerrainOperationStackVersion ||
      stack.nextOperationId == kInvalidCreativeTerrainOperationId ||
      !stack.baseHeightField.validateInvariants() ||
      stack.operations.size() > kCreativeTerrainOperationCapacity) {
    return false;
  }
  CreativeTerrainOperationId maximumId = kInvalidCreativeTerrainOperationId;
  for (std::size_t index = 0U; index < stack.operations.size(); ++index) {
    const CreativeTerrainOperation& operation = stack.operations[index];
    if (operation.id == kInvalidCreativeTerrainOperationId ||
        !isValidCreativeTerrainGeneratorRecipe(operation.generation) ||
        !isValidCreativeTerrainCompositionRecipe(operation.composition)) {
      return false;
    }
    for (std::size_t prior = 0U; prior < index; ++prior) {
      if (stack.operations[prior].id == operation.id) {
        return false;
      }
    }
    maximumId = std::max(maximumId, operation.id);
  }
  return stack.nextOperationId > maximumId;
}

const CreativeTerrainOperation* findCreativeTerrainOperation(
    const CreativeTerrainOperationStack& stack,
    CreativeTerrainOperationId operationId) noexcept {
  const std::size_t index =
      findCreativeTerrainOperationIndex(stack, operationId);
  return index < stack.operations.size() ? &stack.operations[index] : nullptr;
}

std::size_t findCreativeTerrainOperationIndex(
    const CreativeTerrainOperationStack& stack,
    CreativeTerrainOperationId operationId) noexcept {
  const auto found = std::find_if(
      stack.operations.begin(), stack.operations.end(),
      [operationId](const CreativeTerrainOperation& operation) {
        return operation.id == operationId;
      });
  return static_cast<std::size_t>(found - stack.operations.begin());
}

std::string_view toString(
    CreativeTerrainOperationReplayStatus status) noexcept {
  switch (status) {
    case CreativeTerrainOperationReplayStatus::NotRequested:
      return "NotRequested";
    case CreativeTerrainOperationReplayStatus::InvalidTerrain:
      return "InvalidTerrain";
    case CreativeTerrainOperationReplayStatus::InvalidStack:
      return "InvalidStack";
    case CreativeTerrainOperationReplayStatus::GenerationRejected:
      return "GenerationRejected";
    case CreativeTerrainOperationReplayStatus::CompositionRejected:
      return "CompositionRejected";
    case CreativeTerrainOperationReplayStatus::Ready:
      return "Ready";
  }
  return "Unknown";
}

std::string_view toString(
    CreativeTerrainOperationMutationStatus status) noexcept {
  switch (status) {
    case CreativeTerrainOperationMutationStatus::NotRequested:
      return "NotRequested";
    case CreativeTerrainOperationMutationStatus::InvalidTerrain:
      return "InvalidTerrain";
    case CreativeTerrainOperationMutationStatus::InvalidStack:
      return "InvalidStack";
    case CreativeTerrainOperationMutationStatus::InvalidRequest:
      return "InvalidRequest";
    case CreativeTerrainOperationMutationStatus::CapacityExceeded:
      return "CapacityExceeded";
    case CreativeTerrainOperationMutationStatus::IdExhausted:
      return "IdExhausted";
    case CreativeTerrainOperationMutationStatus::NotFound:
      return "NotFound";
    case CreativeTerrainOperationMutationStatus::ReplayRejected:
      return "ReplayRejected";
    case CreativeTerrainOperationMutationStatus::NoChange:
      return "NoChange";
    case CreativeTerrainOperationMutationStatus::Applied:
      return "Applied";
  }
  return "Unknown";
}

CreativeTerrainOperationReplayResult replayCreativeTerrainOperations(
    const CreativeTerrainField& legacyTerrain,
    const CreativeTerrainOperationStack& stack) {
  CreativeTerrainOperationReplayResult result;
  CreativeTerrainOperationReplayReceipt& receipt = result.receipt;
  receipt.requested = true;
  receipt.operationCount = stack.operations.size();
  if (!legacyTerrain.validateInvariants()) {
    setReplayFailure(receipt,
                     CreativeTerrainOperationReplayStatus::InvalidTerrain, 0U,
                     kInvalidCreativeTerrainOperationId,
                     "creative_terrain_operation_legacy_terrain_invalid");
    return result;
  }
  if (!validateCreativeTerrainOperationStack(stack)) {
    setReplayFailure(receipt,
                     CreativeTerrainOperationReplayStatus::InvalidStack, 0U,
                     kInvalidCreativeTerrainOperationId,
                     "creative_terrain_operation_stack_invalid");
    return result;
  }

  CreativeTerrainHeightField current = stack.baseHeightField;
  for (std::size_t index = 0U; index < stack.operations.size(); ++index) {
    const CreativeTerrainOperation& operation = stack.operations[index];
    if (!operation.enabled) {
      ++receipt.disabledOperationCount;
      continue;
    }
    ++receipt.enabledOperationCount;
    const CreativeTerrainGenerationResult generation =
        buildCreativeTerrainGenerationPlan(operation.generation);
    if (!generation.receipt.accepted) {
      setReplayFailure(
          receipt, CreativeTerrainOperationReplayStatus::GenerationRejected,
          index, operation.id,
          "creative_terrain_operation_generation_rejected");
      return result;
    }
    const CreativeTerrainSurfacePlan canonicalSource =
        buildCreativeComposedTerrainSurfacePlan(legacyTerrain, current);
    const CreativeTerrainCompositionResult composition =
        composeCreativeTerrainGeneration(current, canonicalSource, generation,
                                          operation.composition);
    if (!composition.receipt.accepted) {
      setReplayFailure(
          receipt, CreativeTerrainOperationReplayStatus::CompositionRejected,
          index, operation.id,
          "creative_terrain_operation_composition_rejected");
      return result;
    }
    receipt.evaluatedCellCount += generation.receipt.generatedCellCount;
    receipt.modifiedCellCount += composition.receipt.modifiedCellCount;
    receipt.featheredCellCount += composition.receipt.featheredCellCount;
    current = composition.heightField;
  }

  receipt.accepted = true;
  receipt.status = CreativeTerrainOperationReplayStatus::Ready;
  receipt.outputCellCount = current.cellCount();
  receipt.heightHash = heightFieldHash(current);
  receipt.reasonCode = "creative_terrain_operation_replay_ready";
  result.heightField = std::move(current);
  return result;
}

CreativeTerrainOperationMutationPlan planCreativeTerrainOperationMutation(
    const CreativeTerrainField& legacyTerrain,
    const CreativeTerrainHeightField& currentDerived,
    const CreativeTerrainOperationStack& currentStack,
    const CreativeTerrainOperationMutationRequest& request) {
  CreativeTerrainOperationMutationPlan plan;
  CreativeTerrainOperationMutationReceipt& receipt = plan.receipt;
  receipt.requested = true;
  receipt.kind = request.kind;
  receipt.operationId = request.operationId;
  receipt.operationCountBefore = currentStack.operations.size();
  receipt.operationCountAfter = receipt.operationCountBefore;
  if (!legacyTerrain.validateInvariants() ||
      !currentDerived.validateInvariants()) {
    setMutationStatus(receipt,
                      CreativeTerrainOperationMutationStatus::InvalidTerrain,
                      "creative_terrain_operation_mutation_terrain_invalid");
    return plan;
  }
  if (!validateCreativeTerrainOperationStack(currentStack) ||
      (currentStack.operations.empty() &&
       currentStack.baseHeightField.cellCount() != 0U)) {
    setMutationStatus(receipt,
                      CreativeTerrainOperationMutationStatus::InvalidStack,
                      "creative_terrain_operation_mutation_stack_invalid");
    return plan;
  }
  if (!currentStack.operations.empty()) {
    const CreativeTerrainOperationReplayResult currentReplay =
        replayCreativeTerrainOperations(legacyTerrain, currentStack);
    if (!currentReplay.receipt.accepted ||
        !creativeTerrainHeightFieldsEqual(currentReplay.heightField,
                                          currentDerived)) {
      setMutationStatus(
          receipt, CreativeTerrainOperationMutationStatus::InvalidStack,
          "creative_terrain_operation_derived_field_mismatch");
      return plan;
    }
  }
  if (request.kind >= CreativeTerrainOperationMutationKind::Count) {
    setMutationStatus(receipt,
                      CreativeTerrainOperationMutationStatus::InvalidRequest,
                      "creative_terrain_operation_mutation_kind_invalid");
    return plan;
  }

  CreativeTerrainOperationStack staged = currentStack;
  bool changed = false;
  switch (request.kind) {
    case CreativeTerrainOperationMutationKind::Add: {
      if (!requestCarriesValidRecipe(request)) {
        setMutationStatus(
            receipt, CreativeTerrainOperationMutationStatus::InvalidRequest,
            "creative_terrain_operation_add_recipe_invalid");
        return plan;
      }
      if (staged.operations.size() >= kCreativeTerrainOperationCapacity) {
        setMutationStatus(
            receipt,
            CreativeTerrainOperationMutationStatus::CapacityExceeded,
            "creative_terrain_operation_capacity_exceeded");
        return plan;
      }
      if (staged.nextOperationId ==
          std::numeric_limits<CreativeTerrainOperationId>::max()) {
        setMutationStatus(receipt,
                          CreativeTerrainOperationMutationStatus::IdExhausted,
                          "creative_terrain_operation_id_exhausted");
        return plan;
      }
      if (staged.operations.empty()) {
        staged.baseHeightField = currentDerived;
      }
      receipt.operationId = staged.nextOperationId++;
      receipt.operationIndexBefore = staged.operations.size();
      receipt.operationIndexAfter = staged.operations.size();
      staged.operations.push_back({receipt.operationId, request.enabled,
                                   request.generation, request.composition});
      changed = true;
      break;
    }
    case CreativeTerrainOperationMutationKind::Update: {
      if (!requestCarriesValidRecipe(request)) {
        setMutationStatus(
            receipt, CreativeTerrainOperationMutationStatus::InvalidRequest,
            "creative_terrain_operation_update_recipe_invalid");
        return plan;
      }
      const std::size_t index = findCreativeTerrainOperationIndex(
          staged, request.operationId);
      if (index >= staged.operations.size()) {
        setMutationStatus(receipt,
                          CreativeTerrainOperationMutationStatus::NotFound,
                          "creative_terrain_operation_not_found");
        return plan;
      }
      receipt.operationIndexBefore = index;
      receipt.operationIndexAfter = index;
      const CreativeTerrainOperation replacement{
          request.operationId, request.enabled, request.generation,
          request.composition};
      changed = staged.operations[index] != replacement;
      staged.operations[index] = replacement;
      break;
    }
    case CreativeTerrainOperationMutationKind::SetEnabled: {
      const std::size_t index = findCreativeTerrainOperationIndex(
          staged, request.operationId);
      if (index >= staged.operations.size()) {
        setMutationStatus(receipt,
                          CreativeTerrainOperationMutationStatus::NotFound,
                          "creative_terrain_operation_not_found");
        return plan;
      }
      receipt.operationIndexBefore = index;
      receipt.operationIndexAfter = index;
      changed = staged.operations[index].enabled != request.enabled;
      staged.operations[index].enabled = request.enabled;
      break;
    }
    case CreativeTerrainOperationMutationKind::Move: {
      const std::size_t index = findCreativeTerrainOperationIndex(
          staged, request.operationId);
      if (index >= staged.operations.size()) {
        setMutationStatus(receipt,
                          CreativeTerrainOperationMutationStatus::NotFound,
                          "creative_terrain_operation_not_found");
        return plan;
      }
      if (request.targetIndex >= staged.operations.size()) {
        setMutationStatus(
            receipt, CreativeTerrainOperationMutationStatus::InvalidRequest,
            "creative_terrain_operation_target_index_invalid");
        return plan;
      }
      receipt.operationIndexBefore = index;
      receipt.operationIndexAfter = request.targetIndex;
      changed = index != request.targetIndex;
      if (changed) {
        CreativeTerrainOperation moved = staged.operations[index];
        using Difference =
            std::vector<CreativeTerrainOperation>::difference_type;
        staged.operations.erase(
            staged.operations.begin() + static_cast<Difference>(index));
        staged.operations.insert(staged.operations.begin() +
                                     static_cast<Difference>(
                                         request.targetIndex),
                                 std::move(moved));
      }
      break;
    }
    case CreativeTerrainOperationMutationKind::Remove: {
      const std::size_t index = findCreativeTerrainOperationIndex(
          staged, request.operationId);
      if (index >= staged.operations.size()) {
        setMutationStatus(receipt,
                          CreativeTerrainOperationMutationStatus::NotFound,
                          "creative_terrain_operation_not_found");
        return plan;
      }
      receipt.operationIndexBefore = index;
      receipt.operationIndexAfter = index;
      using Difference =
          std::vector<CreativeTerrainOperation>::difference_type;
      staged.operations.erase(
          staged.operations.begin() + static_cast<Difference>(index));
      changed = true;
      break;
    }
    case CreativeTerrainOperationMutationKind::Count:
      break;
  }

  if (!changed) {
    plan.stack = currentStack;
    plan.heightField = currentDerived;
    receipt.accepted = true;
    receipt.operationCountAfter = currentStack.operations.size();
    setMutationStatus(receipt,
                      CreativeTerrainOperationMutationStatus::NoChange,
                      "creative_terrain_operation_mutation_no_change");
    return plan;
  }

  if (staged.operations.empty()) {
    plan.heightField = staged.baseHeightField;
    staged = {};
    receipt.replay.requested = true;
    receipt.replay.accepted = true;
    receipt.replay.status = CreativeTerrainOperationReplayStatus::Ready;
    receipt.replay.outputCellCount = plan.heightField.cellCount();
    receipt.replay.heightHash = heightFieldHash(plan.heightField);
    receipt.replay.reasonCode = "creative_terrain_operation_replay_base_ready";
  } else {
    CreativeTerrainOperationReplayResult replay =
        replayCreativeTerrainOperations(legacyTerrain, staged);
    receipt.replay = replay.receipt;
    if (!replay.receipt.accepted) {
      setMutationStatus(
          receipt, CreativeTerrainOperationMutationStatus::ReplayRejected,
          "creative_terrain_operation_mutation_replay_rejected");
      return plan;
    }
    plan.heightField = std::move(replay.heightField);
  }

  plan.stack = std::move(staged);
  receipt.accepted = true;
  receipt.changed = true;
  receipt.operationCountAfter = plan.stack.operations.size();
  setMutationStatus(receipt, CreativeTerrainOperationMutationStatus::Applied,
                    "creative_terrain_operation_mutation_applied");
  return plan;
}

}  // namespace iggy3d::creative
