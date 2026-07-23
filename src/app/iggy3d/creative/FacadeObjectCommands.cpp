#include "app/iggy3d/creative/Facade.hpp"

#include "app/iggy3d/creative/FacadeInternal.hpp"

#include <algorithm>
#include <limits>
#include <span>
#include <vector>

namespace iggy3d::creative {
namespace {

using facade_internal::objectIdToTargetRef;
using facade_internal::targetRefToObjectId;
using facade_internal::recordCommandAttempt;
using facade_internal::recordCommandFailure;
using facade_internal::recordCommandSuccess;
using facade_internal::recordObjectCreated;
using facade_internal::recordRoomCreated;
using facade_internal::invalidateRemovedObjectEditorState;

[[nodiscard]] std::vector<CreativeObjectId> selectedObjectIds(
    const CreativeSelectionState& selectionState) {
  std::vector<CreativeObjectId> objectIds;
  const std::span<const TargetRef> targets = selectedTargetList(selectionState);
  objectIds.reserve(targets.empty() ? 1U : targets.size());
  if (targets.empty()) {
    CreativeObjectId objectId = kInvalidObjectId;
    if (targetRefToObjectId(selectionState.selectedTarget, objectId)) {
      objectIds.push_back(objectId);
    }
    return objectIds;
  }
  for (TargetRef target : targets) {
    CreativeObjectId objectId = kInvalidObjectId;
    if (targetRefToObjectId(target, objectId)) {
      objectIds.push_back(objectId);
    }
  }
  return objectIds;
}

[[nodiscard]] CreativeFacadeMutationReceipt toggleSelectedObjectMutation(
    CreativeDocument& document,
    TargetRef selectedTarget,
    CreativeMutationKind mutationKind) {
  CreativeFacadeMutationReceipt receipt;
  receipt.requested = true;
  receipt.target = selectedTarget;
  receipt.revisionBefore = document.revision();
  receipt.revisionAfter = receipt.revisionBefore;

  if (receipt.target.value == kInvalidId) {
    receipt.status = CreativeFacadeMutationStatus::NoSelection;
    receipt.message = "no_selection";
    return receipt;
  }

  receipt.hadSelection = true;
  receipt.mutationKind = mutationKind;

  CreativeObjectId objectId = kInvalidObjectId;
  if (!targetRefToObjectId(receipt.target, objectId)) {
    receipt.status = CreativeFacadeMutationStatus::MissingObject;
    receipt.documentStatus = CreativeDocumentMutationStatus::MissingObject;
    receipt.message = "missing_object";
    return receipt;
  }
  receipt.objectId = objectId;

  const CreativeObject* object = document.findObject(objectId);
  if (object == nullptr) {
    receipt.status = CreativeFacadeMutationStatus::MissingObject;
    receipt.documentStatus = CreativeDocumentMutationStatus::MissingObject;
    receipt.message = "missing_object";
    return receipt;
  }

  receipt.objectKind = object->kind;
  receipt.visibleBefore = object->visible;
  receipt.lockedBefore = object->locked;

  const CreativeDocumentMutationReceipt documentReceipt =
      mutationKind == CreativeMutationKind::SetLocked
          ? setDocumentObjectLocked(document, objectId, !receipt.lockedBefore)
          : setDocumentObjectVisible(document, objectId,
                                     !receipt.visibleBefore);

  receipt.accepted = documentMutationSucceeded(documentReceipt.status);
  receipt.changed = documentReceipt.changed &&
                    documentReceipt.revisionAfter !=
                        documentReceipt.revisionBefore;
  receipt.documentStatus = documentReceipt.status;
  receipt.mutationKind = documentReceipt.mutationKind;
  receipt.revisionBefore = documentReceipt.revisionBefore;
  receipt.revisionAfter = documentReceipt.revisionAfter;
  receipt.message = documentReceipt.message;

  const CreativeObject* objectAfter = document.findObject(objectId);
  if (objectAfter != nullptr) {
    receipt.objectKind = objectAfter->kind;
    receipt.visibleAfter = objectAfter->visible;
    receipt.lockedAfter = objectAfter->locked;
  } else {
    receipt.visibleAfter = receipt.visibleBefore;
    receipt.lockedAfter = receipt.lockedBefore;
  }

  if (documentReceipt.status == CreativeDocumentMutationStatus::Applied &&
      receipt.changed) {
    receipt.status = CreativeFacadeMutationStatus::Applied;
  } else if (documentReceipt.status == CreativeDocumentMutationStatus::NoChange) {
    receipt.status = CreativeFacadeMutationStatus::NoChange;
  } else {
    receipt.status = CreativeFacadeMutationStatus::Rejected;
  }

  return receipt;
}

}  // namespace

CreativeFacadeMutationReceipt Facade::toggleSelectedObjectVisibility() {
  return toggleSelectedObjectMutation(document_,
                                      selectionState_.selectedTarget,
                                      CreativeMutationKind::SetVisible);
}

CreativeFacadeMutationReceipt Facade::toggleSelectedObjectLocked() {
  return toggleSelectedObjectMutation(document_,
                                      selectionState_.selectedTarget,
                                      CreativeMutationKind::SetLocked);
}

CreativeTransformCommandReceipt Facade::transformSelectedObjects(
    const CreativeTransformCommandRequest& request) {
  recordCommandAttempt(stats_);
  const std::vector<CreativeObjectId> objectIds =
      selectedObjectIds(selectionState_);
  CreativeTransformCommandReceipt receipt =
      transformDocumentObjectsAtomically(document_, objectIds, request);
  if (receipt.accepted) {
    recordCommandSuccess(stats_);
  } else {
    recordCommandFailure(stats_);
  }
  return receipt;
}

CreativeSelectionPlacementReceipt Facade::placeObjects(
    std::span<const CreativeObjectId> objectIds,
    const CreativeSelectionPlacementRequest& request) {
  recordCommandAttempt(stats_);
  CreativeSelectionPlacementReceipt receipt =
      placeDocumentObjectsAtomically(document_, objectIds, request);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }

  std::vector<TargetRef> targets;
  targets.reserve(objectIds.size());
  for (CreativeObjectId objectId : objectIds) {
    const TargetRef target = objectIdToTargetRef(objectId);
    if (target.value != kInvalidId) {
      targets.push_back(target);
    }
  }
  const TargetRef primary = targets.empty() ? TargetRef{} : targets.back();
  static_cast<void>(setSelectedTargets(selectionState_, targets, primary));
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeSelectionReceipt Facade::selectTargets(
    std::span<const CreativeObjectId> objectIds,
    CreativeObjectId primaryObjectId) {
  recordCommandAttempt(stats_);
  std::vector<TargetRef> targets;
  targets.reserve(objectIds.size());
  for (const CreativeObjectId objectId : objectIds) {
    if (document_.findObject(objectId) == nullptr) {
      continue;
    }
    const TargetRef target = objectIdToTargetRef(objectId);
    if (target.value != kInvalidId) {
      targets.push_back(target);
    }
  }
  TargetRef primary = document_.findObject(primaryObjectId) != nullptr
                          ? objectIdToTargetRef(primaryObjectId)
                          : TargetRef{};
  if (primary.value == kInvalidId && !targets.empty()) {
    primary = targets.back();
  }
  CreativeSelectionReceipt receipt =
      setSelectedTargets(selectionState_, targets, primary);
  if (receipt.accepted) {
    recordCommandSuccess(stats_);
  } else {
    recordCommandFailure(stats_);
  }
  return receipt;
}

CreativeDuplicateCommandReceipt Facade::duplicateSelectedObjects(
    const CreativeDuplicateCommandRequest& request) {
  recordCommandAttempt(stats_);
  const std::vector<CreativeObjectId> objectIds =
      selectedObjectIds(selectionState_);
  if (!objectIds.empty()) {
    const CreativeObjectId nextObjectId = document_.nextObjectId();
    const CreativeObjectId maxTargetId = std::numeric_limits<Id>::max();
    if (nextObjectId > maxTargetId ||
        objectIds.size() - 1U > maxTargetId - nextObjectId) {
      CreativeDuplicateCommandReceipt receipt;
      receipt.requested = true;
      receipt.requestedObjectCount = objectIds.size();
      receipt.status = CreativeDuplicateCommandStatus::InvalidRequest;
      receipt.revisionBefore = document_.revision();
      receipt.revisionAfter = receipt.revisionBefore;
      receipt.message = "duplicate_target_id_exhausted";
      recordCommandFailure(stats_);
      return receipt;
    }
  }
  CreativeDuplicateCommandReceipt receipt =
      duplicateDocumentObjectsAtomically(document_, objectIds, request);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }

  std::vector<TargetRef> duplicateTargets;
  duplicateTargets.reserve(receipt.duplicatedSelectionObjectIds.size());
  for (CreativeObjectId objectId : receipt.duplicatedSelectionObjectIds) {
    const TargetRef target = objectIdToTargetRef(objectId);
    if (target.value != kInvalidId) {
      duplicateTargets.push_back(target);
    }
  }
  for (CreativeObjectId objectId : receipt.duplicatedObjectIds) {
    recordObjectCreated(stats_);
    const CreativeObject* object = document_.findObject(objectId);
    if (object != nullptr && object->kind == CreativeObjectKind::Room) {
      recordRoomCreated(stats_);
    }
  }
  const TargetRef primaryTarget = duplicateTargets.empty()
                                      ? TargetRef{}
                                      : duplicateTargets.back();
  static_cast<void>(setSelectedTargets(selectionState_, duplicateTargets,
                                       primaryTarget));
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeGroupCommandReceipt Facade::groupSelectedObjects() {
  recordCommandAttempt(stats_);
  const std::vector<CreativeObjectId> objectIds =
      selectedObjectIds(selectionState_);
  CreativeGroupCommandReceipt receipt =
      groupDocumentObjectsAtomically(document_, objectIds);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }
  std::vector<TargetRef> targets;
  targets.reserve(receipt.selectionObjectIds.size());
  for (CreativeObjectId objectId : receipt.selectionObjectIds) {
    const TargetRef target = objectIdToTargetRef(objectId);
    if (target.value != kInvalidId) {
      targets.push_back(target);
    }
  }
  const TargetRef primary = targets.empty() ? TargetRef{} : targets.back();
  static_cast<void>(setSelectedTargets(selectionState_, targets, primary));
  recordObjectCreated(stats_);
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeGroupCommandReceipt Facade::ungroupSelectedObject() {
  CreativeObjectId groupObjectId = kInvalidObjectId;
  if (!targetRefToObjectId(selectionState_.selectedTarget, groupObjectId)) {
    recordCommandAttempt(stats_);
    CreativeGroupCommandReceipt receipt;
    receipt.requested = true;
    receipt.kind = CreativeGroupCommandKind::Ungroup;
    receipt.status = CreativeGroupCommandStatus::EmptySelection;
    receipt.revisionBefore = document_.revision();
    receipt.revisionAfter = receipt.revisionBefore;
    receipt.reasonCode = "creative_ungroup_selection_empty";
    recordCommandFailure(stats_);
    return receipt;
  }
  return ungroupObject(groupObjectId);
}

CreativeGroupCommandReceipt Facade::ungroupObject(
    CreativeObjectId groupObjectId) {
  recordCommandAttempt(stats_);
  CreativeGroupCommandReceipt receipt =
      ungroupDocumentObjectAtomically(document_, groupObjectId);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }
  std::vector<TargetRef> targets;
  targets.reserve(receipt.selectionObjectIds.size());
  for (CreativeObjectId objectId : receipt.selectionObjectIds) {
    const TargetRef target = objectIdToTargetRef(objectId);
    if (target.value != kInvalidId) {
      targets.push_back(target);
    }
  }
  const TargetRef primary = targets.empty() ? TargetRef{} : targets.back();
  static_cast<void>(setSelectedTargets(selectionState_, targets, primary));
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeGroupPivotReceipt Facade::setGroupPivot(
    CreativeObjectId groupObjectId,
    CreativeVec3 pivot) {
  recordCommandAttempt(stats_);
  CreativeGroupPivotReceipt receipt =
      setCreativeGroupPivot(document_, groupObjectId, pivot);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeAuthoredAssetInstanceReceipt Facade::instantiateAuthoredAsset(
    const CreativeAuthoredAssetPlacementRequest& request) {
  recordCommandAttempt(stats_);
  CreativeAuthoredAssetInstanceReceipt receipt =
      instantiateCreativeAuthoredAssetAtomically(document_, request);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }
  const TargetRef root = objectIdToTargetRef(receipt.instanceRootObjectId);
  static_cast<void>(setSelectedTargets(selectionState_, std::span{&root, 1U},
                                       root));
  for (std::size_t index = 0U;
       index < receipt.instanceObjectIds.size() + 1U; ++index) {
    recordObjectCreated(stats_);
  }
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeAuthoredAssetRefreshReceipt Facade::refreshAuthoredAssetInstances(
    const CreativeAuthoredAssetDefinition& definition,
    CreativeObjectId preferredInstanceRootObjectId,
    CreativeAuthoredAssetRefreshMode mode) {
  recordCommandAttempt(stats_);
  CreativeAuthoredAssetRefreshRequest request;
  request.definition = &definition;
  request.mode = mode;
  request.selectedInstanceRootObjectId = preferredInstanceRootObjectId;
  CreativeAuthoredAssetRefreshReceipt receipt =
      refreshCreativeAuthoredAssetInstancesAtomically(document_, request);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }

  const bool preferredWasRefreshed =
      std::find(receipt.instanceRootObjectIds.begin(),
                receipt.instanceRootObjectIds.end(),
                preferredInstanceRootObjectId) !=
      receipt.instanceRootObjectIds.end();
  if (preferredWasRefreshed &&
      document_.containsObject(preferredInstanceRootObjectId)) {
    const TargetRef root = objectIdToTargetRef(preferredInstanceRootObjectId);
    static_cast<void>(setSelectedTargets(selectionState_,
                                         std::span{&root, 1U}, root));
  }
  for (std::size_t index = 0U; index < receipt.createdObjectCount; ++index) {
    recordObjectCreated(stats_);
  }
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeDocumentBatchMutationReceipt
Facade::acknowledgeAuthoredAssetInstanceSource(
    const CreativeAuthoredAssetDefinition& definition,
    CreativeObjectId instanceRootObjectId) {
  recordCommandAttempt(stats_);
  CreativeDocumentBatchMutationReceipt receipt =
      acknowledgeCreativeAuthoredAssetInstanceSource(
          document_, definition, instanceRootObjectId);
  if (!receipt.committed || !documentMutationSucceeded(receipt.status)) {
    recordCommandFailure(stats_);
    return receipt;
  }
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeLinearArrayReceipt Facade::createLinearArrayFromSelection(
    const CreativeLinearArrayRequest& request) {
  recordCommandAttempt(stats_);
  const std::vector<CreativeObjectId> objectIds =
      selectedObjectIds(selectionState_);
  const CreativeHierarchySelection hierarchy =
      resolveCreativeObjectHierarchy(document_, objectIds);
  const std::span<const CreativeObjectId> sourceObjectIds =
      hierarchy.accepted
          ? std::span<const CreativeObjectId>{hierarchy.objectIds}
          : std::span<const CreativeObjectId>{objectIds};
  if (!sourceObjectIds.empty()) {
    CreativeLinearArrayPlanRequest planRequest;
    planRequest.sourceObjectCount = sourceObjectIds.size();
    planRequest.direction = request.direction;
    planRequest.copyCount = request.copyCount;
    planRequest.spacing = request.spacing;
    planRequest.cellSize = request.cellSize;
    planRequest.maxGeneratedObjects = request.maxGeneratedObjects;
    const CreativeLinearArrayPlanReceipt plan =
        planCreativeLinearArray(planRequest);
    if (plan.accepted) {
      const CreativeObjectId nextObjectId = document_.nextObjectId();
      const CreativeObjectId maxTargetId = std::numeric_limits<Id>::max();
      if (nextObjectId > maxTargetId ||
          plan.generatedObjectCount - 1U > maxTargetId - nextObjectId) {
        CreativeLinearArrayReceipt receipt;
        receipt.requested = true;
        receipt.requestedObjectCount = objectIds.size();
        receipt.sourceObjectCount = sourceObjectIds.size();
        receipt.generatedObjectCount = plan.generatedObjectCount;
        receipt.status = CreativeLinearArrayStatus::ObjectIdExhausted;
        receipt.revisionBefore = document_.revision();
        receipt.revisionAfter = receipt.revisionBefore;
        receipt.plan = plan;
        receipt.message = "creative_linear_array_target_id_exhausted";
        recordCommandFailure(stats_);
        return receipt;
      }
    }
  }

  CreativeLinearArrayReceipt receipt =
      createCreativeLinearArrayAtomically(document_, sourceObjectIds, request);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }

  for (CreativeObjectId objectId : receipt.generatedObjectIds()) {
    recordObjectCreated(stats_);
    const CreativeObject* object = document_.findObject(objectId);
    if (object != nullptr && object->kind == CreativeObjectKind::Room) {
      recordRoomCreated(stats_);
    }
  }

  std::vector<TargetRef> finalCopyTargets;
  const CreativeHierarchySelection finalCopyHierarchy =
      resolveCreativeObjectHierarchy(document_, receipt.finalCopyObjectIds());
  const std::span<const CreativeObjectId> finalCopySelectionIds =
      finalCopyHierarchy.accepted
          ? std::span<const CreativeObjectId>{
                finalCopyHierarchy.rootObjectIds}
          : receipt.finalCopyObjectIds();
  finalCopyTargets.reserve(finalCopySelectionIds.size());
  for (CreativeObjectId objectId : finalCopySelectionIds) {
    const TargetRef target = objectIdToTargetRef(objectId);
    if (target.value != kInvalidId) {
      finalCopyTargets.push_back(target);
    }
  }
  const TargetRef primaryTarget = finalCopyTargets.empty()
                                      ? TargetRef{}
                                      : finalCopyTargets.back();
  static_cast<void>(setSelectedTargets(selectionState_, finalCopyTargets,
                                       primaryTarget));
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeRadialArrayReceipt Facade::createRadialArrayFromSelection(
    const CreativeRadialArrayRequest& request) {
  recordCommandAttempt(stats_);
  const std::vector<CreativeObjectId> objectIds =
      selectedObjectIds(selectionState_);
  const CreativeHierarchySelection hierarchy =
      resolveCreativeObjectHierarchy(document_, objectIds);
  const std::span<const CreativeObjectId> sourceObjectIds =
      hierarchy.accepted
          ? std::span<const CreativeObjectId>{hierarchy.objectIds}
          : std::span<const CreativeObjectId>{objectIds};
  if (!sourceObjectIds.empty()) {
    CreativeRadialArrayPlanRequest planRequest;
    planRequest.sourceObjectCount = sourceObjectIds.size();
    planRequest.pivot = request.pivot;
    planRequest.axis = request.axis;
    planRequest.instanceCount = request.instanceCount;
    planRequest.sweep = request.sweep;
    planRequest.maxGeneratedObjects = request.maxGeneratedObjects;
    const CreativeRadialArrayPlanReceipt plan =
        planCreativeRadialArray(planRequest);
    if (plan.accepted) {
      const CreativeObjectId nextObjectId = document_.nextObjectId();
      const CreativeObjectId maxTargetId = std::numeric_limits<Id>::max();
      if (nextObjectId > maxTargetId ||
          plan.generatedObjectCount - 1U > maxTargetId - nextObjectId) {
        CreativeRadialArrayReceipt receipt;
        receipt.requested = true;
        receipt.requestedObjectCount = objectIds.size();
        receipt.sourceObjectCount = sourceObjectIds.size();
        receipt.generatedObjectCount = plan.generatedObjectCount;
        receipt.status = CreativeRadialArrayStatus::ObjectIdExhausted;
        receipt.revisionBefore = document_.revision();
        receipt.revisionAfter = receipt.revisionBefore;
        receipt.plan = plan;
        receipt.message = "creative_radial_array_target_id_exhausted";
        recordCommandFailure(stats_);
        return receipt;
      }
    }
  }

  CreativeRadialArrayReceipt receipt =
      createCreativeRadialArrayAtomically(document_, sourceObjectIds, request);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }

  for (CreativeObjectId objectId : receipt.generatedObjectIds()) {
    recordObjectCreated(stats_);
    const CreativeObject* object = document_.findObject(objectId);
    if (object != nullptr && object->kind == CreativeObjectKind::Room) {
      recordRoomCreated(stats_);
    }
  }

  std::vector<TargetRef> finalCopyTargets;
  const CreativeHierarchySelection finalCopyHierarchy =
      resolveCreativeObjectHierarchy(document_, receipt.finalCopyObjectIds());
  const std::span<const CreativeObjectId> finalCopySelectionIds =
      finalCopyHierarchy.accepted
          ? std::span<const CreativeObjectId>{
                finalCopyHierarchy.rootObjectIds}
          : receipt.finalCopyObjectIds();
  finalCopyTargets.reserve(finalCopySelectionIds.size());
  for (CreativeObjectId objectId : finalCopySelectionIds) {
    const TargetRef target = objectIdToTargetRef(objectId);
    if (target.value != kInvalidId) {
      finalCopyTargets.push_back(target);
    }
  }
  const TargetRef primaryTarget = finalCopyTargets.empty()
                                      ? TargetRef{}
                                      : finalCopyTargets.back();
  static_cast<void>(setSelectedTargets(selectionState_, finalCopyTargets,
                                       primaryTarget));
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeLinearArrayReceipt Facade::updateLinearArrayRecipe(
    CreativePatternRecipeId recipeId,
    const CreativeLinearArrayRequest& request) {
  recordCommandAttempt(stats_);
  CreativeLinearArrayReceipt receipt =
      updateCreativeLinearArrayRecipeAtomically(document_, recipeId, request);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }
  for (CreativeObjectId objectId : receipt.generatedObjectIds()) {
    recordObjectCreated(stats_);
    const CreativeObject* object = document_.findObject(objectId);
    if (object != nullptr && object->kind == CreativeObjectKind::Room) {
      recordRoomCreated(stats_);
    }
  }
  const CreativeHierarchySelection finalCopyHierarchy =
      resolveCreativeObjectHierarchy(document_, receipt.finalCopyObjectIds());
  const std::span<const CreativeObjectId> selectedIds =
      finalCopyHierarchy.accepted
          ? std::span<const CreativeObjectId>{
                finalCopyHierarchy.rootObjectIds}
          : receipt.finalCopyObjectIds();
  static_cast<void>(selectTargets(
      selectedIds,
      selectedIds.empty() ? kInvalidObjectId : selectedIds.back()));
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeRadialArrayReceipt Facade::updateRadialArrayRecipe(
    CreativePatternRecipeId recipeId,
    const CreativeRadialArrayRequest& request) {
  recordCommandAttempt(stats_);
  CreativeRadialArrayReceipt receipt =
      updateCreativeRadialArrayRecipeAtomically(document_, recipeId, request);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }
  for (CreativeObjectId objectId : receipt.generatedObjectIds()) {
    recordObjectCreated(stats_);
    const CreativeObject* object = document_.findObject(objectId);
    if (object != nullptr && object->kind == CreativeObjectKind::Room) {
      recordRoomCreated(stats_);
    }
  }
  const CreativeHierarchySelection finalCopyHierarchy =
      resolveCreativeObjectHierarchy(document_, receipt.finalCopyObjectIds());
  const std::span<const CreativeObjectId> selectedIds =
      finalCopyHierarchy.accepted
          ? std::span<const CreativeObjectId>{
                finalCopyHierarchy.rootObjectIds}
          : receipt.finalCopyObjectIds();
  static_cast<void>(selectTargets(
      selectedIds,
      selectedIds.empty() ? kInvalidObjectId : selectedIds.back()));
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeAssetScatterRecipeMutationReceipt Facade::createAssetScatterRecipe(
    std::span<const CreativeDocumentCreateRequest> createRequests,
    std::span<const CreativeObjectId> selectionFilterObjectIds,
    const CreativeAssetScatterRecipe& recipe) {
  recordCommandAttempt(stats_);
  CreativeAssetScatterRecipeMutationReceipt receipt =
      createCreativeAssetScatterRecipeAtomically(
          document_, createRequests, selectionFilterObjectIds, recipe);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }
  for (CreativeObjectId objectId : receipt.generatedObjectIds) {
    recordObjectCreated(stats_);
    const CreativeObject* object = document_.findObject(objectId);
    if (object != nullptr && object->kind == CreativeObjectKind::Room) {
      recordRoomCreated(stats_);
    }
  }
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeAssetScatterRecipeMutationReceipt Facade::updateAssetScatterRecipe(
    CreativePatternRecipeId recipeId,
    std::span<const CreativeDocumentCreateRequest> createRequests,
    const CreativeAssetScatterRecipe& recipe) {
  recordCommandAttempt(stats_);
  CreativeAssetScatterRecipeMutationReceipt receipt =
      updateCreativeAssetScatterRecipeAtomically(document_, recipeId,
                                                 createRequests, recipe);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }
  for (CreativeObjectId objectId : receipt.replacedGeneratedObjectIds) {
    invalidateRemovedObjectEditorState(objectId, toolState_,
                                       selectionState_, measurementState_,
                                       ghostState_);
  }
  for (CreativeObjectId objectId : receipt.generatedObjectIds) {
    recordObjectCreated(stats_);
    const CreativeObject* object = document_.findObject(objectId);
    if (object != nullptr && object->kind == CreativeObjectKind::Room) {
      recordRoomCreated(stats_);
    }
  }
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeAssetScatterRecipeMutationReceipt Facade::removeAssetScatterRecipe(
    CreativePatternRecipeId recipeId) {
  recordCommandAttempt(stats_);
  CreativeAssetScatterRecipeMutationReceipt receipt =
      removeCreativeAssetScatterRecipeAtomically(document_, recipeId);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }
  for (CreativeObjectId objectId : receipt.replacedGeneratedObjectIds) {
    invalidateRemovedObjectEditorState(objectId, toolState_,
                                       selectionState_, measurementState_,
                                       ghostState_);
  }
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeAssetScatterRecipeMutationReceipt Facade::extendAssetScatterRecipe(
    CreativePatternRecipeId recipeId,
    std::span<const CreativeDocumentCreateRequest> createRequests,
    const CreativeAssetScatterRecipe& recipe) {
  recordCommandAttempt(stats_);
  CreativeAssetScatterRecipeMutationReceipt receipt =
      extendCreativeAssetScatterRecipeAtomically(
          document_, recipeId, createRequests, recipe);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }
  for (CreativeObjectId objectId : receipt.generatedObjectIds) {
    recordObjectCreated(stats_);
    const CreativeObject* object = document_.findObject(objectId);
    if (object != nullptr && object->kind == CreativeObjectKind::Room) {
      recordRoomCreated(stats_);
    }
  }
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeAssetScatterRecipeMutationReceipt Facade::excludeAssetScatterOutput(
    CreativePatternRecipeId recipeId,
    CreativeObjectId outputObjectId,
    const CreativeAssetScatterRecipe& recipe) {
  recordCommandAttempt(stats_);
  CreativeAssetScatterRecipeMutationReceipt receipt =
      excludeCreativeAssetScatterOutputAtomically(
          document_, recipeId, outputObjectId, recipe);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }
  for (CreativeObjectId objectId : receipt.replacedGeneratedObjectIds) {
    invalidateRemovedObjectEditorState(objectId, toolState_,
                                       selectionState_, measurementState_,
                                       ghostState_);
  }
  recordCommandSuccess(stats_);
  return receipt;
}

CreativePatternRecipeMutationReceipt Facade::detachPatternRecipe(
    CreativePatternRecipeId recipeId) {
  recordCommandAttempt(stats_);
  CreativePatternRecipeMutationReceipt receipt =
      detachCreativePatternRecipe(document_, recipeId);
  if (!receipt.accepted || !receipt.changed) {
    recordCommandFailure(stats_);
    return receipt;
  }
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeClipboardCopyReceipt Facade::copySelectedObjectsToClipboard(
    CreativeClipboard& outClipboard) {
  recordCommandAttempt(stats_);
  const std::vector<CreativeObjectId> objectIds =
      selectedObjectIds(selectionState_);
  const CreativeHierarchySelection hierarchy =
      resolveCreativeObjectHierarchy(document_, objectIds);
  CreativeClipboardCopyReceipt receipt = copyDocumentObjectsToClipboard(
      document_, hierarchy.accepted ? hierarchy.objectIds : objectIds,
      outClipboard);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeClipboardCutReceipt Facade::cutSelectedObjectsToClipboard(
    CreativeClipboard& outClipboard) {
  recordCommandAttempt(stats_);
  const std::vector<CreativeObjectId> objectIds =
      selectedObjectIds(selectionState_);
  const CreativeHierarchySelection hierarchy =
      resolveCreativeObjectHierarchy(document_, objectIds);
  const std::span<const CreativeObjectId> cutIds =
      hierarchy.accepted
          ? std::span<const CreativeObjectId>{hierarchy.objectIds}
          : std::span<const CreativeObjectId>{objectIds};
  CreativeClipboardCutReceipt receipt =
      cutDocumentObjectsAtomically(document_, cutIds, outClipboard);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }
  for (const CreativeObject& object : outClipboard.objects) {
    invalidateRemovedObjectEditorState(object.id, toolState_,
                                       selectionState_, measurementState_,
                                       ghostState_);
  }
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeClipboardPasteReceipt Facade::pasteClipboard(
    const CreativeClipboard& clipboard,
    const CreativeClipboardPasteRequest& request) {
  recordCommandAttempt(stats_);
  CreativeClipboardPasteReceipt receipt =
      pasteCreativeClipboardAtomically(document_, clipboard, request);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }

  std::vector<TargetRef> pastedTargets;
  const CreativeHierarchySelection pastedHierarchy =
      resolveCreativeObjectHierarchy(document_, receipt.pastedObjectIds);
  const std::span<const CreativeObjectId> pastedSelectionIds =
      pastedHierarchy.accepted
          ? std::span<const CreativeObjectId>{pastedHierarchy.rootObjectIds}
          : std::span<const CreativeObjectId>{receipt.pastedObjectIds};
  pastedTargets.reserve(pastedSelectionIds.size());
  for (CreativeObjectId objectId : pastedSelectionIds) {
    const TargetRef target = objectIdToTargetRef(objectId);
    if (target.value != kInvalidId) {
      pastedTargets.push_back(target);
    }
  }
  for (CreativeObjectId objectId : receipt.pastedObjectIds) {
    recordObjectCreated(stats_);
    const CreativeObject* object = document_.findObject(objectId);
    if (object != nullptr && object->kind == CreativeObjectKind::Room) {
      recordRoomCreated(stats_);
    }
  }
  const TargetRef primary =
      pastedTargets.empty() ? TargetRef{} : pastedTargets.back();
  static_cast<void>(setSelectedTargets(selectionState_, pastedTargets, primary));
  recordCommandSuccess(stats_);
  return receipt;
}

}  // namespace iggy3d::creative
