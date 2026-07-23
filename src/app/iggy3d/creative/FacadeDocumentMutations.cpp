#include "app/iggy3d/creative/Facade.hpp"

#include "app/iggy3d/creative/FacadeInternal.hpp"

#include <span>
#include <vector>

namespace iggy3d::creative {

using facade_internal::objectIdToTargetRef;
using facade_internal::recordCommandAttempt;
using facade_internal::recordCommandFailure;
using facade_internal::recordCommandSuccess;
using facade_internal::recordObjectCreated;
using facade_internal::recordRoomCreated;
using facade_internal::invalidateRemovedObjectEditorState;

CreativeVolumeOperationReceipt Facade::applyVolumeOperation(
    const CreativeVolumeOperationRequest& request) {
  recordCommandAttempt(stats_);
  CreativeVolumeOperationReceipt receipt =
      executeCreativeVolumeOperation(document_, request);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }

  for (CreativeObjectId objectId : receipt.removedObjectIds) {
    invalidateRemovedObjectEditorState(objectId, toolState_,
                                       selectionState_, measurementState_,
                                       ghostState_);
  }

  std::vector<TargetRef> createdTargets;
  createdTargets.reserve(receipt.createdObjectIds.size());
  for (CreativeObjectId objectId : receipt.createdObjectIds) {
    const TargetRef target = objectIdToTargetRef(objectId);
    if (target.value != kInvalidId) {
      createdTargets.push_back(target);
    }
    recordObjectCreated(stats_);
    const CreativeObject* object = document_.findObject(objectId);
    if (object != nullptr && object->kind == CreativeObjectKind::Room) {
      recordRoomCreated(stats_);
    }
  }
  if (!createdTargets.empty()) {
    const TargetRef primary = createdTargets.back();
    static_cast<void>(
        setSelectedTargets(selectionState_, createdTargets, primary));
  }

  recordCommandSuccess(stats_);
  return receipt;
}

CreativeVoxelMutationReceipt Facade::applyVoxelEdits(
    std::span<const CreativeVoxelEdit> edits) {
  recordCommandAttempt(stats_);
  CreativeVoxelMutationReceipt receipt = document_.applyVoxelEdits(edits);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeTerrainMutationReceipt Facade::applyTerrainControlEdits(
    std::span<const CreativeTerrainControlEdit> edits) {
  recordCommandAttempt(stats_);
  CreativeTerrainMutationReceipt receipt =
      document_.applyTerrainControlEdits(edits);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeTerrainHeightFieldReplaceReceipt Facade::replaceTerrainHeightField(
    CreativeTerrainHeightFieldBounds bounds,
    std::span<const std::uint16_t> heights) {
  recordCommandAttempt(stats_);
  CreativeTerrainHeightFieldReplaceReceipt receipt =
      document_.replaceTerrainHeightField(bounds, heights);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeTerrainOperationMutationReceipt Facade::applyTerrainOperationMutation(
    const CreativeTerrainOperationMutationRequest& request) {
  recordCommandAttempt(stats_);
  CreativeTerrainOperationMutationReceipt receipt =
      document_.applyTerrainOperationMutation(request);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }
  recordCommandSuccess(stats_);
  return receipt;
}

CreativePatternRecipeTranslationReceipt Facade::applyPatternRecipeTranslation(
    const CreativePatternRecipeTranslationPlan& plan) {
  recordCommandAttempt(stats_);
  CreativePatternRecipeTranslationReceipt receipt =
      applyCreativePatternRecipeTranslation(document_, plan);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeTerrainOperationTranslationReceipt
Facade::applyTerrainOperationTranslation(
    const CreativeTerrainOperationTranslationPlan& plan) {
  recordCommandAttempt(stats_);
  CreativeTerrainOperationTranslationReceipt receipt =
      applyCreativeTerrainOperationTranslation(document_, plan);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeTerrainMaterialMutationReceipt Facade::applyTerrainMaterialEdits(
    std::span<const CreativeTerrainMaterialEdit> edits) {
  recordCommandAttempt(stats_);
  CreativeTerrainMaterialMutationReceipt receipt =
      document_.applyTerrainMaterialEdits(edits);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeDocumentCreateReceipt Facade::createDocumentObject(
    const CreativeDocumentCreateRequest& request) {
  recordCommandAttempt(stats_);
  CreativeDocumentCreateReceipt receipt = document_.createObject(request);
  if (!receipt.accepted || !receipt.objectCreated) {
    recordCommandFailure(stats_);
    return receipt;
  }

  recordCommandSuccess(stats_);
  recordObjectCreated(stats_);
  if (receipt.objectKind == CreativeObjectKind::Room) {
    recordRoomCreated(stats_);
  }
  return receipt;
}

CreativeDocumentCreateReceipt Facade::createDocumentObject(
    CreativeObjectKind kind) {
  CreativeDocumentCreateRequest request;
  request.kind = kind;
  return createDocumentObject(request);
}

CreativeDocumentRemoveReceipt Facade::removeDocumentObject(
    const CreativeDocumentRemoveRequest& request) {
  recordCommandAttempt(stats_);
  const CreativeObject* requestedObject =
      document_.findObject(request.objectId);
  bool requestedObjectHasChildren = false;
  if (requestedObject != nullptr) {
    for (const CreativeObject& candidate : document_.objects()) {
      if (candidate.parentId == requestedObject->id) {
        requestedObjectHasChildren = true;
        break;
      }
    }
  }
  if (requestedObject != nullptr &&
      (creativeObjectIsHierarchyContainer(requestedObject->kind) ||
       requestedObjectHasChildren)) {
    CreativeHierarchyRemoveReceipt hierarchy =
        removeCreativeObjectHierarchyAtomically(document_, request.objectId);
    if (!hierarchy.accepted) {
      CreativeDocumentRemoveReceipt rejected;
      rejected.requested = true;
      rejected.objectId = request.objectId;
      rejected.objectKind = requestedObject->kind;
      rejected.objectName = requestedObject->name;
      rejected.revisionBefore = hierarchy.revisionBefore;
      rejected.revisionAfter = hierarchy.revisionAfter;
      const CreativeObject* failedObject =
          document_.findObject(hierarchy.failedObjectId);
      rejected.status = failedObject != nullptr && failedObject->locked
                            ? CreativeDocumentRemoveStatus::LockedObject
                            : CreativeDocumentRemoveStatus::ParentHasChildren;
      rejected.message = hierarchy.reasonCode;
      rejected.reasonCode = hierarchy.reasonCode;
      recordCommandFailure(stats_);
      return rejected;
    }
    for (CreativeObjectId objectId : hierarchy.removedObjectIds) {
      invalidateRemovedObjectEditorState(objectId, toolState_,
                                         selectionState_, measurementState_,
                                         ghostState_);
    }
    recordCommandSuccess(stats_);
    return hierarchy.rootReceipt;
  }
  CreativeDocumentRemoveReceipt receipt = document_.removeDocumentObject(request);
  if (!receipt.accepted || !receipt.objectRemoved) {
    recordCommandFailure(stats_);
    return receipt;
  }

  invalidateRemovedObjectEditorState(receipt.objectId,
                                     toolState_,
                                     selectionState_,
                                     measurementState_,
                                     ghostState_);
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeDocumentRemoveReceipt Facade::removeDocumentObject(
    CreativeObjectId id) {
  CreativeDocumentRemoveRequest request;
  request.objectId = id;
  return removeDocumentObject(request);
}

CreativeLogicLinkMutationReceipt Facade::setLogicLink(
    const CreativeLogicLinkMutationRequest& request) {
  recordCommandAttempt(stats_);
  CreativeLogicLinkMutationReceipt receipt = document_.setLogicLink(request);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeLogicLinkMutationReceipt Facade::removeLogicLink(
    CreativeObjectId sourceObjectId,
    CreativeObjectId targetObjectId) {
  recordCommandAttempt(stats_);
  CreativeLogicLinkMutationReceipt receipt =
      document_.removeLogicLink(sourceObjectId, targetObjectId);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeHierarchyBatchRemoveReceipt Facade::removeDocumentObjectsAtomically(
    std::span<const CreativeObjectId> objectIds) {
  recordCommandAttempt(stats_);
  CreativeHierarchyBatchRemoveReceipt receipt =
      removeCreativeObjectHierarchiesAtomically(document_, objectIds);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }
  for (CreativeObjectId objectId : receipt.removedObjectIds) {
    invalidateRemovedObjectEditorState(objectId, toolState_,
                                       selectionState_, measurementState_,
                                       ghostState_);
  }
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeSemanticDeleteReceipt Facade::deleteDocumentObjectsSemantically(
    std::span<const CreativeObjectId> objectIds) {
  recordCommandAttempt(stats_);
  CreativeSemanticDeleteReceipt receipt =
      deleteDocumentObjectsSemanticallyAtomically(document_, objectIds);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }
  for (CreativeObjectId objectId : receipt.removedObjectIds) {
    invalidateRemovedObjectEditorState(objectId, toolState_,
                                       selectionState_, measurementState_,
                                       ghostState_);
  }
  recordCommandSuccess(stats_);
  return receipt;
}

}  // namespace iggy3d::creative
