#include "app/iggy3d/creative/tools/Pattern.hpp"

#include "app/iggy3d/creative/tools/Group.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>
#include <unordered_set>
#include <utility>

namespace iggy3d::creative {
namespace {

template <typename Enum>
[[nodiscard]] bool validEnum(Enum value, Enum count) noexcept {
  return static_cast<std::size_t>(value) <
         static_cast<std::size_t>(count);
}

template <typename Receipt>
void normalizeArrayReceiptRevisionRange(
    Receipt& receipt,
    std::uint64_t revisionAfter) noexcept {
  receipt.revisionAfter = revisionAfter;
  if (receipt.pasteReceipt.requested) {
    receipt.pasteReceipt.revisionBefore = receipt.revisionBefore;
    receipt.pasteReceipt.revisionAfter = revisionAfter;
  }
  if (receipt.patternMutationReceipt.requested) {
    receipt.patternMutationReceipt.revisionBefore = receipt.revisionBefore;
    receipt.patternMutationReceipt.revisionAfter = revisionAfter;
  }
}

template <typename Receipt>
void clearUnpublishedArrayOutputs(Receipt& receipt,
                                  bool clearRecipeId) noexcept {
  receipt.pasteReceipt.pastedObjectIds.clear();
  receipt.pasteReceipt.idRemaps.clear();
  receipt.pasteReceipt.patternRecipeIdRemaps.clear();
  receipt.generatedObjectCount = 0U;
  receipt.finalCopyFirstObjectIndex = 0U;
  receipt.finalCopyObjectCount = 0U;
  if (clearRecipeId) {
    receipt.patternRecipeId = kInvalidCreativePatternRecipeId;
  }
}

[[nodiscard]] CreativeVec3 directionUnit(
    CreativeLinearArrayDirection direction) noexcept {
  switch (direction) {
    case CreativeLinearArrayDirection::PositiveX: return {1.0, 0.0, 0.0};
    case CreativeLinearArrayDirection::NegativeX: return {-1.0, 0.0, 0.0};
    case CreativeLinearArrayDirection::PositiveY: return {0.0, 1.0, 0.0};
    case CreativeLinearArrayDirection::NegativeY: return {0.0, -1.0, 0.0};
    case CreativeLinearArrayDirection::PositiveZ: return {0.0, 0.0, 1.0};
    case CreativeLinearArrayDirection::NegativeZ: return {0.0, 0.0, -1.0};
    case CreativeLinearArrayDirection::Count: break;
  }
  return {};
}

[[nodiscard]] CreativeLinearArrayStatus statusForClipboardFailure(
    CreativeClipboardStatus status) noexcept {
  switch (status) {
    case CreativeClipboardStatus::EmptySelection:
      return CreativeLinearArrayStatus::EmptySelection;
    case CreativeClipboardStatus::MissingObject:
      return CreativeLinearArrayStatus::MissingObject;
    case CreativeClipboardStatus::ObjectIdExhausted:
      return CreativeLinearArrayStatus::ObjectIdExhausted;
    case CreativeClipboardStatus::NotRequested:
    case CreativeClipboardStatus::InvalidClipboard:
    case CreativeClipboardStatus::InvalidRequest:
    case CreativeClipboardStatus::CreateRejected:
    case CreativeClipboardStatus::RemoveRejected:
    case CreativeClipboardStatus::Copied:
    case CreativeClipboardStatus::Cut:
    case CreativeClipboardStatus::Pasted:
      return CreativeLinearArrayStatus::PasteRejected;
  }
  return CreativeLinearArrayStatus::PasteRejected;
}

[[nodiscard]] CreativeRadialArrayStatus radialStatusForClipboardFailure(
    CreativeClipboardStatus status) noexcept {
  switch (status) {
    case CreativeClipboardStatus::EmptySelection:
      return CreativeRadialArrayStatus::EmptySelection;
    case CreativeClipboardStatus::MissingObject:
      return CreativeRadialArrayStatus::MissingObject;
    case CreativeClipboardStatus::ObjectIdExhausted:
      return CreativeRadialArrayStatus::ObjectIdExhausted;
    case CreativeClipboardStatus::NotRequested:
    case CreativeClipboardStatus::InvalidClipboard:
    case CreativeClipboardStatus::InvalidRequest:
    case CreativeClipboardStatus::CreateRejected:
    case CreativeClipboardStatus::RemoveRejected:
    case CreativeClipboardStatus::Copied:
    case CreativeClipboardStatus::Cut:
    case CreativeClipboardStatus::Pasted:
      return CreativeRadialArrayStatus::PasteRejected;
  }
  return CreativeRadialArrayStatus::PasteRejected;
}

}  // namespace

std::string_view toString(CreativeLinearArrayStatus status) noexcept {
  switch (status) {
    case CreativeLinearArrayStatus::NotRequested: return "NotRequested";
    case CreativeLinearArrayStatus::EmptySelection: return "EmptySelection";
    case CreativeLinearArrayStatus::InvalidRequest: return "InvalidRequest";
    case CreativeLinearArrayStatus::OperationLimitExceeded:
      return "OperationLimitExceeded";
    case CreativeLinearArrayStatus::MissingObject: return "MissingObject";
    case CreativeLinearArrayStatus::ObjectIdExhausted:
      return "ObjectIdExhausted";
    case CreativeLinearArrayStatus::PasteRejected: return "PasteRejected";
    case CreativeLinearArrayStatus::RecipeNotFound: return "RecipeNotFound";
    case CreativeLinearArrayStatus::RecipeKindMismatch:
      return "RecipeKindMismatch";
    case CreativeLinearArrayStatus::RecipeDependencyConflict:
      return "RecipeDependencyConflict";
    case CreativeLinearArrayStatus::RecipeRejected: return "RecipeRejected";
    case CreativeLinearArrayStatus::RemoveRejected: return "RemoveRejected";
    case CreativeLinearArrayStatus::Planned: return "Planned";
    case CreativeLinearArrayStatus::Applied: return "Applied";
  }
  return "Unknown";
}

std::string_view toString(CreativeRadialArrayStatus status) noexcept {
  switch (status) {
    case CreativeRadialArrayStatus::NotRequested: return "NotRequested";
    case CreativeRadialArrayStatus::EmptySelection: return "EmptySelection";
    case CreativeRadialArrayStatus::InvalidRequest: return "InvalidRequest";
    case CreativeRadialArrayStatus::DegenerateRadius:
      return "DegenerateRadius";
    case CreativeRadialArrayStatus::OperationLimitExceeded:
      return "OperationLimitExceeded";
    case CreativeRadialArrayStatus::MissingObject: return "MissingObject";
    case CreativeRadialArrayStatus::ObjectIdExhausted:
      return "ObjectIdExhausted";
    case CreativeRadialArrayStatus::PasteRejected: return "PasteRejected";
    case CreativeRadialArrayStatus::RecipeNotFound: return "RecipeNotFound";
    case CreativeRadialArrayStatus::RecipeKindMismatch:
      return "RecipeKindMismatch";
    case CreativeRadialArrayStatus::RecipeDependencyConflict:
      return "RecipeDependencyConflict";
    case CreativeRadialArrayStatus::RecipeRejected: return "RecipeRejected";
    case CreativeRadialArrayStatus::RemoveRejected: return "RemoveRejected";
    case CreativeRadialArrayStatus::Planned: return "Planned";
    case CreativeRadialArrayStatus::Applied: return "Applied";
  }
  return "Unknown";
}

CreativeLinearArrayPlanReceipt planCreativeLinearArray(
    const CreativeLinearArrayPlanRequest& request) noexcept {
  CreativeLinearArrayPlanReceipt receipt;
  receipt.requested = true;
  receipt.sourceObjectCount = request.sourceObjectCount;
  if (request.sourceObjectCount == 0U) {
    receipt.status = CreativeLinearArrayStatus::EmptySelection;
    receipt.reasonCode = "creative_linear_array_selection_empty";
    return receipt;
  }
  if (!validEnum(request.direction, CreativeLinearArrayDirection::Count) ||
      !validEnum(request.copyCount, CreativeLinearArrayCopyCount::Count) ||
      !validEnum(request.spacing, CreativeLinearArraySpacing::Count) ||
      !std::isfinite(request.cellSize) || request.cellSize <= 0.0 ||
      request.maxGeneratedObjects == 0U ||
      request.maxGeneratedObjects >
          kCreativeLinearArrayGeneratedObjectCapacity) {
    receipt.status = CreativeLinearArrayStatus::InvalidRequest;
    receipt.reasonCode = "creative_linear_array_request_invalid";
    return receipt;
  }

  const std::uint64_t copyCount =
      creativeLinearArrayCopyCountValue(request.copyCount);
  const std::uint64_t spacingCells =
      creativeLinearArraySpacingCells(request.spacing);
  receipt.copyCount = copyCount;
  if (copyCount == 0U || spacingCells == 0U ||
      request.sourceObjectCount >
          std::numeric_limits<std::uint64_t>::max() / copyCount) {
    receipt.status = CreativeLinearArrayStatus::InvalidRequest;
    receipt.reasonCode = "creative_linear_array_size_overflow";
    return receipt;
  }
  receipt.generatedObjectCount = request.sourceObjectCount * copyCount;
  if (receipt.generatedObjectCount > request.maxGeneratedObjects) {
    receipt.status = CreativeLinearArrayStatus::OperationLimitExceeded;
    receipt.reasonCode = "creative_linear_array_limit_exceeded";
    return receipt;
  }

  const CreativeVec3 unit = directionUnit(request.direction);
  for (std::uint64_t ordinal = 1U; ordinal <= copyCount; ++ordinal) {
    const double distance = static_cast<double>(ordinal) *
                            static_cast<double>(spacingCells) *
                            request.cellSize;
    if (!std::isfinite(distance)) {
      receipt.status = CreativeLinearArrayStatus::InvalidRequest;
      receipt.reasonCode = "creative_linear_array_distance_invalid";
      receipt.instanceCount = 0U;
      return receipt;
    }
    CreativeLinearArrayInstance& instance =
        receipt.instances[receipt.instanceCount++];
    instance.ordinal = static_cast<std::uint32_t>(ordinal);
    instance.offset = {unit.x * distance, unit.y * distance,
                       unit.z * distance};
  }

  receipt.accepted = true;
  receipt.status = CreativeLinearArrayStatus::Planned;
  receipt.reasonCode = "creative_linear_array_planned";
  return receipt;
}

CreativeRadialArrayPlanReceipt planCreativeRadialArray(
    const CreativeRadialArrayPlanRequest& request) noexcept {
  CreativeRadialArrayPlanReceipt receipt;
  receipt.requested = true;
  receipt.sourceObjectCount = request.sourceObjectCount;
  receipt.pivot = request.pivot;
  receipt.axis = request.axis;
  if (request.sourceObjectCount == 0U) {
    receipt.status = CreativeRadialArrayStatus::EmptySelection;
    receipt.reasonCode = "creative_radial_array_selection_empty";
    return receipt;
  }
  if (!isFiniteCreativeVec3(request.pivot) ||
      !isValidCreativeAxis3(request.axis) ||
      !validEnum(request.instanceCount,
                 CreativeRadialArrayInstanceCount::Count) ||
      !validEnum(request.sweep, CreativeRadialArraySweep::Count) ||
      request.maxGeneratedObjects == 0U ||
      request.maxGeneratedObjects >
          kCreativeRadialArrayGeneratedObjectCapacity) {
    receipt.status = CreativeRadialArrayStatus::InvalidRequest;
    receipt.reasonCode = "creative_radial_array_request_invalid";
    return receipt;
  }

  const std::uint64_t totalInstanceCount =
      creativeRadialArrayInstanceCountValue(request.instanceCount);
  const std::uint64_t generatedCopyCount = totalInstanceCount - 1U;
  receipt.totalInstanceCount = totalInstanceCount;
  receipt.generatedCopyCount = generatedCopyCount;
  if (totalInstanceCount < 2U ||
      generatedCopyCount > kCreativeRadialArrayInstanceCapacity ||
      request.sourceObjectCount >
          std::numeric_limits<std::uint64_t>::max() / generatedCopyCount) {
    receipt.status = CreativeRadialArrayStatus::InvalidRequest;
    receipt.reasonCode = "creative_radial_array_size_overflow";
    return receipt;
  }
  receipt.generatedObjectCount =
      request.sourceObjectCount * generatedCopyCount;
  if (receipt.generatedObjectCount > request.maxGeneratedObjects) {
    receipt.status = CreativeRadialArrayStatus::OperationLimitExceeded;
    receipt.reasonCode = "creative_radial_array_limit_exceeded";
    return receipt;
  }

  const double sweepRadians = creativeRadialArraySweepDegrees(request.sweep) *
                              std::numbers::pi / 180.0;
  const bool closedRing =
      request.sweep == CreativeRadialArraySweep::Degrees360;
  const double divisor = static_cast<double>(
      closedRing ? totalInstanceCount : generatedCopyCount);
  for (std::uint64_t ordinal = 1U; ordinal <= generatedCopyCount; ++ordinal) {
    const double angle = sweepRadians * static_cast<double>(ordinal) / divisor;
    if (!std::isfinite(angle)) {
      receipt.status = CreativeRadialArrayStatus::InvalidRequest;
      receipt.reasonCode = "creative_radial_array_angle_invalid";
      receipt.instanceCount = 0U;
      return receipt;
    }
    CreativeRadialArrayInstance& instance =
        receipt.instances[receipt.instanceCount++];
    instance.ordinal = static_cast<std::uint32_t>(ordinal);
    instance.angleRadians = angle;
  }

  receipt.accepted = true;
  receipt.status = CreativeRadialArrayStatus::Planned;
  receipt.reasonCode = "creative_radial_array_planned";
  return receipt;
}

CreativePatternReplacementPreflight preflightCreativePatternReplacement(
    const CreativeDocument& document,
    const CreativePatternRecipe& recipe) {
  CreativePatternReplacementPreflight result;
  std::unordered_set<CreativeObjectId> generatedIds{
      recipe.generatedObjectIds.begin(), recipe.generatedObjectIds.end()};
  result.rootObjectIds.reserve(recipe.generatedObjectIds.size());
  for (CreativeObjectId objectId : recipe.generatedObjectIds) {
    const CreativeObject* object = document.findObject(objectId);
    if (object == nullptr) {
      result.failedObjectId = objectId;
      result.reasonCode = "creative_pattern_generated_object_missing";
      return result;
    }
    if (!object->parentId.has_value() ||
        !generatedIds.contains(*object->parentId)) {
      result.rootObjectIds.push_back(objectId);
    }
  }

  for (const CreativeObject& object : document.objects()) {
    if (object.parentId.has_value() &&
        generatedIds.contains(*object.parentId) &&
        !generatedIds.contains(object.id)) {
      result.failedObjectId = object.id;
      result.reasonCode = "creative_pattern_external_child_attached";
      return result;
    }
  }
  for (const CreativePatternRecipe& other :
       document.patternRecipeStore().recipes) {
    if (other.id == recipe.id) {
      continue;
    }
    const auto dependency = std::find_if(
        other.sourceObjectIds.begin(), other.sourceObjectIds.end(),
        [&generatedIds](CreativeObjectId objectId) {
          return generatedIds.contains(objectId);
        });
    if (dependency != other.sourceObjectIds.end()) {
      result.failedObjectId = *dependency;
      result.reasonCode = "creative_pattern_output_has_dependent_recipe";
      return result;
    }
  }
  if (result.rootObjectIds.empty()) {
    result.reasonCode = "creative_pattern_generated_roots_missing";
    return result;
  }
  result.accepted = true;
  result.reasonCode = "creative_pattern_replacement_ready";
  return result;
}

namespace {

void unlockPatternOutputs(CreativeDocument& document,
                          std::span<const CreativeObjectId> objectIds) {
  for (CreativeObjectId objectId : objectIds) {
    CreativeObject* object = document.findObject(objectId);
    if (object != nullptr) {
      object->locked = false;
    }
  }
}

CreativeLinearArrayReceipt createCreativeLinearArrayCopiesAtomically(
    CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds,
    const CreativeLinearArrayRequest& request) {
  CreativeLinearArrayReceipt receipt;
  receipt.requested = true;
  receipt.requestedObjectCount = objectIds.size();
  receipt.revisionBefore = document.revision();
  receipt.revisionAfter = receipt.revisionBefore;
  if (objectIds.empty()) {
    receipt.status = CreativeLinearArrayStatus::EmptySelection;
    receipt.message = "creative_linear_array_selection_empty";
    return receipt;
  }

  CreativeClipboard clipboard;
  receipt.copyReceipt =
      copyDocumentObjectsToClipboard(document, objectIds, clipboard,
                                     CreativeClipboardCopyMode::ExactObjects);
  if (!receipt.copyReceipt.accepted) {
    receipt.failedObjectId = receipt.copyReceipt.failedObjectId;
    receipt.status = statusForClipboardFailure(receipt.copyReceipt.status);
    receipt.message = receipt.copyReceipt.reasonCode;
    return receipt;
  }
  receipt.sourceObjectCount = receipt.copyReceipt.copiedObjectCount;
  receipt.sourceObjectIds.reserve(clipboard.objects.size());
  for (const CreativeObject& object : clipboard.objects) {
    receipt.sourceObjectIds.push_back(object.id);
  }

  CreativeLinearArrayPlanRequest planRequest;
  planRequest.sourceObjectCount = receipt.sourceObjectCount;
  planRequest.direction = request.direction;
  planRequest.copyCount = request.copyCount;
  planRequest.spacing = request.spacing;
  planRequest.cellSize = request.cellSize;
  planRequest.maxGeneratedObjects = request.maxGeneratedObjects;
  receipt.plan = planCreativeLinearArray(planRequest);
  receipt.generatedObjectCount = receipt.plan.generatedObjectCount;
  if (!receipt.plan.accepted) {
    receipt.status = receipt.plan.status;
    receipt.message = receipt.plan.reasonCode;
    return receipt;
  }

  std::array<CreativeClipboardPasteRequest,
             kCreativeLinearArrayInstanceCapacity>
      pasteRequests{};
  for (std::size_t index = 0; index < receipt.plan.instanceCount; ++index) {
    pasteRequests[index].offset = receipt.plan.instances[index].offset;
    pasteRequests[index].appendCopySuffix = true;
    pasteRequests[index].externalParentPolicy =
        CreativeClipboardExternalParentPolicy::PreserveIfPresent;
  }
  receipt.pasteReceipt = pasteCreativeClipboardBatchAtomically(
      document, clipboard,
      std::span<const CreativeClipboardPasteRequest>{pasteRequests.data(),
                                                      receipt.plan.instanceCount});
  if (!receipt.pasteReceipt.accepted) {
    receipt.failedObjectId = receipt.pasteReceipt.failedObjectId;
    receipt.status = statusForClipboardFailure(receipt.pasteReceipt.status);
    receipt.message = receipt.pasteReceipt.reasonCode;
    return receipt;
  }

  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = CreativeLinearArrayStatus::Applied;
  receipt.generatedObjectCount = receipt.pasteReceipt.pastedObjectCount;
  receipt.revisionAfter = document.revision();
  receipt.finalCopyObjectCount =
      static_cast<std::size_t>(receipt.sourceObjectCount);
  receipt.finalCopyFirstObjectIndex =
      receipt.pasteReceipt.pastedObjectIds.size() -
      receipt.finalCopyObjectCount;
  receipt.message = "creative_linear_array_applied";
  return receipt;
}

CreativeRadialArrayReceipt createCreativeRadialArrayCopiesAtomically(
    CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds,
    const CreativeRadialArrayRequest& request) {
  CreativeRadialArrayReceipt receipt;
  receipt.requested = true;
  receipt.requestedObjectCount = objectIds.size();
  receipt.revisionBefore = document.revision();
  receipt.revisionAfter = receipt.revisionBefore;
  if (objectIds.empty()) {
    receipt.status = CreativeRadialArrayStatus::EmptySelection;
    receipt.message = "creative_radial_array_selection_empty";
    return receipt;
  }

  CreativeClipboard clipboard;
  receipt.copyReceipt =
      copyDocumentObjectsToClipboard(document, objectIds, clipboard,
                                     CreativeClipboardCopyMode::ExactObjects);
  if (!receipt.copyReceipt.accepted) {
    receipt.failedObjectId = receipt.copyReceipt.failedObjectId;
    receipt.status = radialStatusForClipboardFailure(receipt.copyReceipt.status);
    receipt.message = receipt.copyReceipt.reasonCode;
    return receipt;
  }
  receipt.sourceObjectCount = receipt.copyReceipt.copiedObjectCount;
  receipt.sourceObjectIds.reserve(clipboard.objects.size());
  for (const CreativeObject& object : clipboard.objects) {
    receipt.sourceObjectIds.push_back(object.id);
  }

  CreativeRadialArrayPlanRequest planRequest;
  planRequest.sourceObjectCount = receipt.sourceObjectCount;
  planRequest.pivot = request.pivot;
  planRequest.axis = request.axis;
  planRequest.instanceCount = request.instanceCount;
  planRequest.sweep = request.sweep;
  planRequest.maxGeneratedObjects = request.maxGeneratedObjects;
  receipt.plan = planCreativeRadialArray(planRequest);
  receipt.generatedObjectCount = receipt.plan.generatedObjectCount;
  if (!receipt.plan.accepted) {
    receipt.status = receipt.plan.status;
    receipt.message = receipt.plan.reasonCode;
    return receipt;
  }

  const double radiusSquared = creativeSquaredDistanceFromAxis(
      clipboard.placementAnchor, request.pivot, request.axis);
  if (!std::isfinite(radiusSquared) || radiusSquared <= 1.0e-12) {
    receipt.status = CreativeRadialArrayStatus::DegenerateRadius;
    receipt.message = "creative_radial_array_radius_degenerate";
    return receipt;
  }

  std::array<CreativeClipboardPasteRequest,
             kCreativeRadialArrayInstanceCapacity>
      pasteRequests{};
  for (std::size_t index = 0; index < receipt.plan.instanceCount; ++index) {
    CreativeClipboardPasteRequest& paste = pasteRequests[index];
    paste.offset = {};
    paste.hasTransformAnchor = true;
    paste.transformAnchor = request.pivot;
    paste.hasAxisAngleRotation = true;
    paste.rotationAxis = request.axis;
    paste.rotationRadians = receipt.plan.instances[index].angleRadians;
    paste.appendCopySuffix = true;
    paste.externalParentPolicy =
        CreativeClipboardExternalParentPolicy::PreserveIfPresent;
  }
  receipt.pasteReceipt = pasteCreativeClipboardBatchAtomically(
      document, clipboard,
      std::span<const CreativeClipboardPasteRequest>{
          pasteRequests.data(), receipt.plan.instanceCount});
  if (!receipt.pasteReceipt.accepted) {
    receipt.failedObjectId = receipt.pasteReceipt.failedObjectId;
    receipt.status =
        radialStatusForClipboardFailure(receipt.pasteReceipt.status);
    receipt.message = receipt.pasteReceipt.reasonCode;
    return receipt;
  }

  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = CreativeRadialArrayStatus::Applied;
  receipt.generatedObjectCount = receipt.pasteReceipt.pastedObjectCount;
  receipt.revisionAfter = document.revision();
  receipt.finalCopyObjectCount =
      static_cast<std::size_t>(receipt.sourceObjectCount);
  receipt.finalCopyFirstObjectIndex =
      receipt.pasteReceipt.pastedObjectIds.size() -
      receipt.finalCopyObjectCount;
  receipt.message = "creative_radial_array_applied";
  return receipt;
}

}  // namespace

CreativeLinearArrayReceipt createCreativeLinearArrayAtomically(
    CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds,
    const CreativeLinearArrayRequest& request) {
  CreativeDocument staged = document;
  CreativeLinearArrayReceipt receipt =
      createCreativeLinearArrayCopiesAtomically(staged, objectIds, request);
  if (!receipt.accepted) {
    normalizeArrayReceiptRevisionRange(receipt, receipt.revisionBefore);
    return receipt;
  }

  CreativePatternRecipe recipe;
  recipe.kind = CreativePatternRecipeKind::LinearArray;
  recipe.sourceObjectIds = receipt.sourceObjectIds;
  recipe.generatedObjectIds.assign(receipt.generatedObjectIds().begin(),
                                   receipt.generatedObjectIds().end());
  recipe.linear = request;
  CreativePatternRecipeMutationRequest mutation;
  mutation.kind = CreativePatternRecipeMutationKind::Add;
  mutation.recipe = std::move(recipe);
  receipt.patternMutationReceipt =
      staged.applyPatternRecipeMutation(mutation);
  if (!receipt.patternMutationReceipt.accepted ||
      !receipt.patternMutationReceipt.changed) {
    receipt.accepted = false;
    receipt.changed = false;
    receipt.status = CreativeLinearArrayStatus::RecipeRejected;
    receipt.patternRecipeId = kInvalidCreativePatternRecipeId;
    receipt.revisionAfter = receipt.revisionBefore;
    receipt.message = std::string{receipt.patternMutationReceipt.reasonCode};
    clearUnpublishedArrayOutputs(receipt, true);
    normalizeArrayReceiptRevisionRange(receipt, receipt.revisionBefore);
    return receipt;
  }

  receipt.patternRecipeId = receipt.patternMutationReceipt.recipeId;
  const CreativeDocumentPublicationReceipt publication =
      document.commitStagedMutation(std::move(staged));
  normalizeArrayReceiptRevisionRange(
      receipt, publication.accepted ? publication.revisionAfter
                                    : receipt.revisionBefore);
  if (!publication.accepted) {
    receipt.accepted = false;
    receipt.changed = false;
    receipt.status = CreativeLinearArrayStatus::RecipeRejected;
    receipt.revisionAfter = receipt.revisionBefore;
    receipt.message = std::string{publication.reasonCode};
    clearUnpublishedArrayOutputs(receipt, true);
    return receipt;
  }
  return receipt;
}

CreativeRadialArrayReceipt createCreativeRadialArrayAtomically(
    CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds,
    const CreativeRadialArrayRequest& request) {
  CreativeDocument staged = document;
  CreativeRadialArrayReceipt receipt =
      createCreativeRadialArrayCopiesAtomically(staged, objectIds, request);
  if (!receipt.accepted) {
    normalizeArrayReceiptRevisionRange(receipt, receipt.revisionBefore);
    return receipt;
  }

  CreativePatternRecipe recipe;
  recipe.kind = CreativePatternRecipeKind::RadialArray;
  recipe.sourceObjectIds = receipt.sourceObjectIds;
  recipe.generatedObjectIds.assign(receipt.generatedObjectIds().begin(),
                                   receipt.generatedObjectIds().end());
  recipe.radial = request;
  CreativePatternRecipeMutationRequest mutation;
  mutation.kind = CreativePatternRecipeMutationKind::Add;
  mutation.recipe = std::move(recipe);
  receipt.patternMutationReceipt =
      staged.applyPatternRecipeMutation(mutation);
  if (!receipt.patternMutationReceipt.accepted ||
      !receipt.patternMutationReceipt.changed) {
    receipt.accepted = false;
    receipt.changed = false;
    receipt.status = CreativeRadialArrayStatus::RecipeRejected;
    receipt.patternRecipeId = kInvalidCreativePatternRecipeId;
    receipt.revisionAfter = receipt.revisionBefore;
    receipt.message = std::string{receipt.patternMutationReceipt.reasonCode};
    clearUnpublishedArrayOutputs(receipt, true);
    normalizeArrayReceiptRevisionRange(receipt, receipt.revisionBefore);
    return receipt;
  }

  receipt.patternRecipeId = receipt.patternMutationReceipt.recipeId;
  const CreativeDocumentPublicationReceipt publication =
      document.commitStagedMutation(std::move(staged));
  normalizeArrayReceiptRevisionRange(
      receipt, publication.accepted ? publication.revisionAfter
                                    : receipt.revisionBefore);
  if (!publication.accepted) {
    receipt.accepted = false;
    receipt.changed = false;
    receipt.status = CreativeRadialArrayStatus::RecipeRejected;
    receipt.revisionAfter = receipt.revisionBefore;
    receipt.message = std::string{publication.reasonCode};
    clearUnpublishedArrayOutputs(receipt, true);
    return receipt;
  }
  return receipt;
}

CreativeLinearArrayReceipt updateCreativeLinearArrayRecipeAtomically(
    CreativeDocument& document,
    CreativePatternRecipeId recipeId,
    const CreativeLinearArrayRequest& request) {
  CreativeLinearArrayReceipt rejected;
  rejected.requested = true;
  rejected.patternRecipeId = recipeId;
  rejected.revisionBefore = document.revision();
  rejected.revisionAfter = rejected.revisionBefore;
  const CreativePatternRecipe* existing =
      findCreativePatternRecipe(document.patternRecipeStore(), recipeId);
  if (existing == nullptr) {
    rejected.status = CreativeLinearArrayStatus::RecipeNotFound;
    rejected.message = "creative_linear_array_recipe_not_found";
    return rejected;
  }
  if (existing->kind != CreativePatternRecipeKind::LinearArray) {
    rejected.status = CreativeLinearArrayStatus::RecipeKindMismatch;
    rejected.message = "creative_linear_array_recipe_kind_mismatch";
    return rejected;
  }
  const CreativePatternReplacementPreflight preflight =
      preflightCreativePatternReplacement(document, *existing);
  if (!preflight.accepted) {
    rejected.failedObjectId = preflight.failedObjectId;
    rejected.status =
        CreativeLinearArrayStatus::RecipeDependencyConflict;
    rejected.message = std::string{preflight.reasonCode};
    return rejected;
  }

  const std::vector<CreativeObjectId> oldGeneratedObjectIds =
      existing->generatedObjectIds;
  const std::vector<CreativeObjectId> sourceObjectIds =
      existing->sourceObjectIds;
  CreativeDocument staged = document;
  CreativeLinearArrayReceipt receipt =
      createCreativeLinearArrayCopiesAtomically(staged, sourceObjectIds,
                                                 request);
  receipt.patternRecipeId = recipeId;
  receipt.replacedGeneratedObjectCount = oldGeneratedObjectIds.size();
  if (!receipt.accepted) {
    normalizeArrayReceiptRevisionRange(receipt, receipt.revisionBefore);
    return receipt;
  }

  CreativePatternRecipe replacement;
  replacement.kind = CreativePatternRecipeKind::LinearArray;
  replacement.sourceObjectIds = receipt.sourceObjectIds;
  replacement.generatedObjectIds.assign(receipt.generatedObjectIds().begin(),
                                        receipt.generatedObjectIds().end());
  replacement.linear = request;
  CreativePatternRecipeMutationRequest mutation;
  mutation.kind = CreativePatternRecipeMutationKind::Replace;
  mutation.recipeId = recipeId;
  mutation.recipe = std::move(replacement);
  receipt.patternMutationReceipt =
      staged.applyPatternRecipeMutation(mutation);
  if (!receipt.patternMutationReceipt.accepted ||
      !receipt.patternMutationReceipt.changed) {
    receipt.accepted = false;
    receipt.changed = false;
    receipt.status = CreativeLinearArrayStatus::RecipeRejected;
    receipt.revisionAfter = receipt.revisionBefore;
    receipt.message = std::string{receipt.patternMutationReceipt.reasonCode};
    clearUnpublishedArrayOutputs(receipt, false);
    normalizeArrayReceiptRevisionRange(receipt, receipt.revisionBefore);
    return receipt;
  }

  unlockPatternOutputs(staged, oldGeneratedObjectIds);
  const CreativeHierarchyBatchRemoveReceipt removed =
      removeCreativeObjectHierarchiesAtomically(
          staged, preflight.rootObjectIds);
  if (!removed.accepted || !removed.changed ||
      removed.removedObjectIds.size() != oldGeneratedObjectIds.size()) {
    receipt.accepted = false;
    receipt.changed = false;
    receipt.failedObjectId = removed.failedObjectId;
    receipt.status = CreativeLinearArrayStatus::RemoveRejected;
    receipt.revisionAfter = receipt.revisionBefore;
    receipt.message = std::string{removed.reasonCode};
    clearUnpublishedArrayOutputs(receipt, false);
    normalizeArrayReceiptRevisionRange(receipt, receipt.revisionBefore);
    return receipt;
  }

  const CreativeDocumentPublicationReceipt publication =
      document.commitStagedMutation(std::move(staged));
  normalizeArrayReceiptRevisionRange(
      receipt, publication.accepted ? publication.revisionAfter
                                    : receipt.revisionBefore);
  if (!publication.accepted) {
    receipt.accepted = false;
    receipt.changed = false;
    receipt.status = CreativeLinearArrayStatus::RemoveRejected;
    receipt.revisionAfter = receipt.revisionBefore;
    receipt.message = std::string{publication.reasonCode};
    clearUnpublishedArrayOutputs(receipt, false);
    return receipt;
  }
  receipt.updatedExistingRecipe = true;
  receipt.message = "creative_linear_array_updated";
  return receipt;
}

CreativeRadialArrayReceipt updateCreativeRadialArrayRecipeAtomically(
    CreativeDocument& document,
    CreativePatternRecipeId recipeId,
    const CreativeRadialArrayRequest& request) {
  CreativeRadialArrayReceipt rejected;
  rejected.requested = true;
  rejected.patternRecipeId = recipeId;
  rejected.revisionBefore = document.revision();
  rejected.revisionAfter = rejected.revisionBefore;
  const CreativePatternRecipe* existing =
      findCreativePatternRecipe(document.patternRecipeStore(), recipeId);
  if (existing == nullptr) {
    rejected.status = CreativeRadialArrayStatus::RecipeNotFound;
    rejected.message = "creative_radial_array_recipe_not_found";
    return rejected;
  }
  if (existing->kind != CreativePatternRecipeKind::RadialArray) {
    rejected.status = CreativeRadialArrayStatus::RecipeKindMismatch;
    rejected.message = "creative_radial_array_recipe_kind_mismatch";
    return rejected;
  }
  const CreativePatternReplacementPreflight preflight =
      preflightCreativePatternReplacement(document, *existing);
  if (!preflight.accepted) {
    rejected.failedObjectId = preflight.failedObjectId;
    rejected.status =
        CreativeRadialArrayStatus::RecipeDependencyConflict;
    rejected.message = std::string{preflight.reasonCode};
    return rejected;
  }

  const std::vector<CreativeObjectId> oldGeneratedObjectIds =
      existing->generatedObjectIds;
  const std::vector<CreativeObjectId> sourceObjectIds =
      existing->sourceObjectIds;
  CreativeDocument staged = document;
  CreativeRadialArrayReceipt receipt =
      createCreativeRadialArrayCopiesAtomically(staged, sourceObjectIds,
                                                 request);
  receipt.patternRecipeId = recipeId;
  receipt.replacedGeneratedObjectCount = oldGeneratedObjectIds.size();
  if (!receipt.accepted) {
    normalizeArrayReceiptRevisionRange(receipt, receipt.revisionBefore);
    return receipt;
  }

  CreativePatternRecipe replacement;
  replacement.kind = CreativePatternRecipeKind::RadialArray;
  replacement.sourceObjectIds = receipt.sourceObjectIds;
  replacement.generatedObjectIds.assign(receipt.generatedObjectIds().begin(),
                                        receipt.generatedObjectIds().end());
  replacement.radial = request;
  CreativePatternRecipeMutationRequest mutation;
  mutation.kind = CreativePatternRecipeMutationKind::Replace;
  mutation.recipeId = recipeId;
  mutation.recipe = std::move(replacement);
  receipt.patternMutationReceipt =
      staged.applyPatternRecipeMutation(mutation);
  if (!receipt.patternMutationReceipt.accepted ||
      !receipt.patternMutationReceipt.changed) {
    receipt.accepted = false;
    receipt.changed = false;
    receipt.status = CreativeRadialArrayStatus::RecipeRejected;
    receipt.revisionAfter = receipt.revisionBefore;
    receipt.message = std::string{receipt.patternMutationReceipt.reasonCode};
    clearUnpublishedArrayOutputs(receipt, false);
    normalizeArrayReceiptRevisionRange(receipt, receipt.revisionBefore);
    return receipt;
  }

  unlockPatternOutputs(staged, oldGeneratedObjectIds);
  const CreativeHierarchyBatchRemoveReceipt removed =
      removeCreativeObjectHierarchiesAtomically(
          staged, preflight.rootObjectIds);
  if (!removed.accepted || !removed.changed ||
      removed.removedObjectIds.size() != oldGeneratedObjectIds.size()) {
    receipt.accepted = false;
    receipt.changed = false;
    receipt.failedObjectId = removed.failedObjectId;
    receipt.status = CreativeRadialArrayStatus::RemoveRejected;
    receipt.revisionAfter = receipt.revisionBefore;
    receipt.message = std::string{removed.reasonCode};
    clearUnpublishedArrayOutputs(receipt, false);
    normalizeArrayReceiptRevisionRange(receipt, receipt.revisionBefore);
    return receipt;
  }

  const CreativeDocumentPublicationReceipt publication =
      document.commitStagedMutation(std::move(staged));
  normalizeArrayReceiptRevisionRange(
      receipt, publication.accepted ? publication.revisionAfter
                                    : receipt.revisionBefore);
  if (!publication.accepted) {
    receipt.accepted = false;
    receipt.changed = false;
    receipt.status = CreativeRadialArrayStatus::RemoveRejected;
    receipt.revisionAfter = receipt.revisionBefore;
    receipt.message = std::string{publication.reasonCode};
    clearUnpublishedArrayOutputs(receipt, false);
    return receipt;
  }
  receipt.updatedExistingRecipe = true;
  receipt.message = "creative_radial_array_updated";
  return receipt;
}

CreativePatternRecipeMutationReceipt detachCreativePatternRecipe(
    CreativeDocument& document,
    CreativePatternRecipeId recipeId) {
  CreativePatternRecipeMutationRequest request;
  request.kind = CreativePatternRecipeMutationKind::Detach;
  request.recipeId = recipeId;
  return document.applyPatternRecipeMutation(request);
}

}  // namespace iggy3d::creative
