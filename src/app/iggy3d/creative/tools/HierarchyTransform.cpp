#include "app/iggy3d/creative/tools/HierarchyTransform.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/tools/SelectionPlacement.hpp"

#include <algorithm>
#include <optional>
#include <span>
#include <string_view>

namespace iggy3d::creative {
namespace {

[[nodiscard]] CreativeHierarchyTransformReceipt makeTransformReceipt(
    const CreativeDocument& document,
    CreativeObjectId rootObjectId) {
  CreativeHierarchyTransformReceipt receipt;
  receipt.requested = true;
  receipt.rootObjectId = rootObjectId;
  receipt.revisionBefore = document.revision();
  receipt.revisionAfter = receipt.revisionBefore;
  return receipt;
}

[[nodiscard]] CreativeHierarchyReattachmentReceipt makeReattachmentReceipt(
    const CreativeDocument& document,
    const CreativeHierarchyReattachmentRequest& request) {
  CreativeHierarchyReattachmentReceipt receipt;
  receipt.requested = true;
  receipt.sourceRootObjectId = request.sourceRootObjectId;
  receipt.targetObjectId = request.targetObjectId;
  receipt.revisionBefore = document.revision();
  receipt.revisionAfter = receipt.revisionBefore;
  return receipt;
}

void reject(CreativeHierarchyTransformReceipt& receipt,
            CreativeHierarchyTransformStatus status,
            CreativeHierarchyTransformPhase phase,
            std::string_view reasonCode,
            std::string_view message,
            CreativeObjectId failedObjectId = kInvalidObjectId) {
  receipt.status = status;
  receipt.phase = phase;
  receipt.failedObjectId = failedObjectId;
  receipt.reasonCode = reasonCode;
  receipt.message = message;
}

void reject(CreativeHierarchyReattachmentReceipt& receipt,
            CreativeHierarchyTransformStatus status,
            CreativeHierarchyTransformPhase phase,
            std::string_view reasonCode,
            std::string_view message,
            CreativeObjectId failedObjectId = kInvalidObjectId) {
  receipt.status = status;
  receipt.phase = phase;
  receipt.failedObjectId = failedObjectId;
  receipt.reasonCode = reasonCode;
  receipt.message = message;
}

[[nodiscard]] bool validRequestedTransform(
    const CreativeHierarchyTransformRequest& request) noexcept {
  if (!request.setPosition && !request.setRotation && !request.setScale) {
    return false;
  }
  return (!request.setPosition ||
          isFiniteCreativeVec3(request.targetTransform.position)) &&
         (!request.setRotation ||
          isFiniteCreativeVec3(request.targetTransform.rotationEulerRadians)) &&
         (!request.setScale || isPositiveCreativeVec3(request.targetTransform.scale));
}

[[nodiscard]] CreativeHierarchyTransformStatus hierarchyStatus(
    CreativeHierarchySelectionStatus status) noexcept {
  switch (status) {
    case CreativeHierarchySelectionStatus::MissingObject:
      return CreativeHierarchyTransformStatus::MissingObject;
    case CreativeHierarchySelectionStatus::InvalidDocument:
      return CreativeHierarchyTransformStatus::InvalidDocument;
    case CreativeHierarchySelectionStatus::InvalidHierarchy:
      return CreativeHierarchyTransformStatus::InvalidHierarchy;
    case CreativeHierarchySelectionStatus::NotRequested:
    case CreativeHierarchySelectionStatus::EmptySelection:
    case CreativeHierarchySelectionStatus::Ready:
      return CreativeHierarchyTransformStatus::InvalidRequest;
  }
  return CreativeHierarchyTransformStatus::InvalidRequest;
}

struct StageResult {
  bool accepted = false;
  bool changed = false;
  CreativeHierarchyTransformPhase phase = CreativeHierarchyTransformPhase::None;
  CreativeObjectId failedObjectId = kInvalidObjectId;
  std::string reasonCode = "creative_hierarchy_transform_not_requested";
  std::string message = "creative_hierarchy_transform_not_requested";
};

enum class ExternalParentPolicy {
  Preserve,
  Detach,
};

[[nodiscard]] StageResult stageAbsoluteHierarchyTransform(
    CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds,
    CreativeObjectId rootObjectId,
    const CreativeTransform& targetTransform,
    bool setPosition,
    bool setRotation,
    bool setScale,
    ExternalParentPolicy externalParentPolicy) {
  StageResult result;
  const CreativeObject* root = document.findObject(rootObjectId);
  if (root == nullptr) {
    result.phase = CreativeHierarchyTransformPhase::ResolveHierarchy;
    result.failedObjectId = rootObjectId;
    result.reasonCode = "creative_hierarchy_transform_root_missing";
    result.message = result.reasonCode;
    return result;
  }

  const std::optional<CreativeObjectId> originalParent = root->parentId;
  const std::string originalAttachmentSocket = root->attachmentSocket;
  const bool hasExternalParent =
      originalParent.has_value() &&
      std::find(objectIds.begin(), objectIds.end(), *root->parentId) ==
          objectIds.end();
  if (hasExternalParent) {
    const CreativeDocumentMutationReceipt detached = applyDocumentMutation(
        document, rootObjectId, CreativeMutationKind::DetachFrom,
        CreativeMutationPayload{});
    if (!documentMutationSucceeded(detached.status)) {
      result.phase = CreativeHierarchyTransformPhase::DetachSource;
      result.failedObjectId = rootObjectId;
      result.reasonCode = detached.message;
      result.message = detached.message;
      return result;
    }
  }

  const auto applyPlacement =
      [&](const CreativeSelectionPlacementRequest& request,
          CreativeHierarchyTransformPhase phase) {
        const CreativeSelectionPlacementReceipt placement =
            placeDocumentObjectsAtomically(document, objectIds, request);
        if (!placement.accepted) {
          result.phase = phase;
          result.failedObjectId = placement.failedObjectId;
          result.reasonCode = placement.reasonCode;
          result.message = placement.reasonCode;
          return false;
        }
        result.changed = result.changed || placement.changed;
        return true;
      };

  const CreativeObject* current = document.findObject(rootObjectId);
  if (current == nullptr || !isPositiveCreativeVec3(current->transform.scale)) {
    result.phase = CreativeHierarchyTransformPhase::Scale;
    result.failedObjectId = rootObjectId;
    result.reasonCode = "creative_hierarchy_transform_root_invalid";
    result.message = result.reasonCode;
    return result;
  }

  if (setScale) {
    const CreativeVec3 scaleFactor{
        targetTransform.scale.x / current->transform.scale.x,
        targetTransform.scale.y / current->transform.scale.y,
        targetTransform.scale.z / current->transform.scale.z};
    if (!isPositiveCreativeVec3(scaleFactor)) {
      result.phase = CreativeHierarchyTransformPhase::Scale;
      result.failedObjectId = rootObjectId;
      result.reasonCode = "creative_hierarchy_transform_scale_invalid";
      result.message = result.reasonCode;
      return result;
    }
    if (!creativeVec3ExactlyEqual(scaleFactor, {1.0, 1.0, 1.0})) {
      CreativeSelectionPlacementRequest scale;
      scale.mode = CreativeSelectionPlacementMode::Move;
      scale.sourceAnchor = current->transform.position;
      scale.targetAnchor = scale.sourceAnchor;
      scale.scaleFactor = scaleFactor;
      if (!applyPlacement(scale, CreativeHierarchyTransformPhase::Scale)) {
        return result;
      }
    }
  }

  if (setRotation) {
    current = document.findObject(rootObjectId);
    if (current == nullptr) {
      result.phase = CreativeHierarchyTransformPhase::ResetRotation;
      result.failedObjectId = rootObjectId;
      result.reasonCode = "creative_hierarchy_transform_root_missing";
      result.message = result.reasonCode;
      return result;
    }
    const CreativeVec3 pivot = current->transform.position;
    const CreativeVec3 currentRotation = current->transform.rotationEulerRadians;
    const auto rotateAroundPivot =
        [&](CreativeAxis3 axis, double radians,
            CreativeHierarchyTransformPhase phase) {
          if (radians == 0.0) {
            return true;
          }
          CreativeSelectionPlacementRequest rotation;
          rotation.mode = CreativeSelectionPlacementMode::Move;
          rotation.sourceAnchor = pivot;
          rotation.targetAnchor = pivot;
          rotation.hasAxisAngleRotation = true;
          rotation.rotationAxis = axis;
          rotation.rotationRadians = radians;
          return applyPlacement(rotation, phase);
        };
    if (!rotateAroundPivot(CreativeAxis3::Z, -currentRotation.z,
                           CreativeHierarchyTransformPhase::ResetRotation) ||
        !rotateAroundPivot(CreativeAxis3::Y, -currentRotation.y,
                           CreativeHierarchyTransformPhase::ResetRotation) ||
        !rotateAroundPivot(CreativeAxis3::X, -currentRotation.x,
                           CreativeHierarchyTransformPhase::ResetRotation) ||
        !rotateAroundPivot(CreativeAxis3::X,
                           targetTransform.rotationEulerRadians.x,
                           CreativeHierarchyTransformPhase::ApplyRotation) ||
        !rotateAroundPivot(CreativeAxis3::Y,
                           targetTransform.rotationEulerRadians.y,
                           CreativeHierarchyTransformPhase::ApplyRotation) ||
        !rotateAroundPivot(CreativeAxis3::Z,
                           targetTransform.rotationEulerRadians.z,
                           CreativeHierarchyTransformPhase::ApplyRotation)) {
      return result;
    }
  }

  if (setPosition) {
    current = document.findObject(rootObjectId);
    if (current == nullptr) {
      result.phase = CreativeHierarchyTransformPhase::Move;
      result.failedObjectId = rootObjectId;
      result.reasonCode = "creative_hierarchy_transform_root_missing";
      result.message = result.reasonCode;
      return result;
    }
    if (!creativeVec3ExactlyEqual(current->transform.position,
                                  targetTransform.position)) {
      CreativeSelectionPlacementRequest move;
      move.mode = CreativeSelectionPlacementMode::Move;
      move.sourceAnchor = current->transform.position;
      move.targetAnchor = targetTransform.position;
      if (!applyPlacement(move, CreativeHierarchyTransformPhase::Move)) {
        return result;
      }
    }
  }

  if (externalParentPolicy == ExternalParentPolicy::Preserve &&
      originalParent.has_value() && hasExternalParent) {
    const CreativeDocumentMutationReceipt restored = applyDocumentMutation(
        document, rootObjectId, CreativeMutationKind::AttachTo,
        makeAttachPayload(*originalParent, originalAttachmentSocket));
    if (!documentMutationSucceeded(restored.status)) {
      result.phase = CreativeHierarchyTransformPhase::Relationship;
      result.failedObjectId = rootObjectId;
      result.reasonCode = restored.message;
      result.message = restored.message;
      return result;
    }
  }

  result.accepted = true;
  result.phase = CreativeHierarchyTransformPhase::Commit;
  result.reasonCode = result.changed ? "creative_hierarchy_transform_staged"
                                      : "creative_hierarchy_transform_no_change";
  result.message = result.reasonCode;
  return result;
}

}  // namespace

std::string_view toString(CreativeHierarchyTransformStatus status) noexcept {
  switch (status) {
    case CreativeHierarchyTransformStatus::NotRequested:
      return "NotRequested";
    case CreativeHierarchyTransformStatus::InvalidDocument:
      return "InvalidDocument";
    case CreativeHierarchyTransformStatus::InvalidRequest:
      return "InvalidRequest";
    case CreativeHierarchyTransformStatus::MissingObject:
      return "MissingObject";
    case CreativeHierarchyTransformStatus::InvalidHierarchy:
      return "InvalidHierarchy";
    case CreativeHierarchyTransformStatus::LockedObject:
      return "LockedObject";
    case CreativeHierarchyTransformStatus::InvalidTransform:
      return "InvalidTransform";
    case CreativeHierarchyTransformStatus::TargetInsideSourceHierarchy:
      return "TargetInsideSourceHierarchy";
    case CreativeHierarchyTransformStatus::StalePlan:
      return "StalePlan";
    case CreativeHierarchyTransformStatus::MutationRejected:
      return "MutationRejected";
    case CreativeHierarchyTransformStatus::NoChange:
      return "NoChange";
    case CreativeHierarchyTransformStatus::Applied:
      return "Applied";
    case CreativeHierarchyTransformStatus::Rejected:
      return "Rejected";
  }
  return "Unknown";
}

std::string_view toString(CreativeHierarchyTransformPhase phase) noexcept {
  switch (phase) {
    case CreativeHierarchyTransformPhase::None: return "None";
    case CreativeHierarchyTransformPhase::ValidateRequest:
      return "ValidateRequest";
    case CreativeHierarchyTransformPhase::ResolveHierarchy:
      return "ResolveHierarchy";
    case CreativeHierarchyTransformPhase::ValidateLocks:
      return "ValidateLocks";
    case CreativeHierarchyTransformPhase::DetachSource:
      return "DetachSource";
    case CreativeHierarchyTransformPhase::Scale: return "Scale";
    case CreativeHierarchyTransformPhase::ResetRotation:
      return "ResetRotation";
    case CreativeHierarchyTransformPhase::ApplyRotation:
      return "ApplyRotation";
    case CreativeHierarchyTransformPhase::Move: return "Move";
    case CreativeHierarchyTransformPhase::Relationship:
      return "Relationship";
    case CreativeHierarchyTransformPhase::Commit: return "Commit";
  }
  return "None";
}

CreativeHierarchyTransformReceipt applyCreativeHierarchyTransformAtomically(
    CreativeDocument& document,
    const CreativeHierarchyTransformRequest& request) {
  CreativeHierarchyTransformReceipt receipt =
      makeTransformReceipt(document, request.rootObjectId);
  if (!document.isValid()) {
    reject(receipt, CreativeHierarchyTransformStatus::InvalidDocument,
           CreativeHierarchyTransformPhase::ValidateRequest,
           "creative_hierarchy_transform_document_invalid",
           "creative_hierarchy_transform_document_invalid");
    return receipt;
  }
  if (request.rootObjectId == kInvalidObjectId ||
      !validRequestedTransform(request)) {
    reject(receipt, CreativeHierarchyTransformStatus::InvalidRequest,
           CreativeHierarchyTransformPhase::ValidateRequest,
           "creative_hierarchy_transform_request_invalid",
           "creative_hierarchy_transform_request_invalid");
    return receipt;
  }

  const CreativeHierarchySelection hierarchy = resolveCreativeObjectHierarchy(
      document, std::span{&request.rootObjectId, 1U});
  if (!hierarchy.accepted) {
    reject(receipt, hierarchyStatus(hierarchy.status),
           CreativeHierarchyTransformPhase::ResolveHierarchy,
           hierarchy.reasonCode, hierarchy.reasonCode, hierarchy.missingObjectId);
    return receipt;
  }
  receipt.hierarchyObjectCount = hierarchy.objectIds.size();
  for (CreativeObjectId objectId : hierarchy.objectIds) {
    if (creativeObjectEffectivelyLocked(document, objectId)) {
      reject(receipt, CreativeHierarchyTransformStatus::LockedObject,
             CreativeHierarchyTransformPhase::ValidateLocks,
             "creative_hierarchy_transform_object_locked",
             "creative_hierarchy_transform_object_locked", objectId);
      return receipt;
    }
  }

  CreativeDocument staged = document;
  const StageResult stagedResult = stageAbsoluteHierarchyTransform(
      staged, hierarchy.objectIds, request.rootObjectId, request.targetTransform,
      request.setPosition, request.setRotation, request.setScale,
      ExternalParentPolicy::Preserve);
  if (!stagedResult.accepted) {
    reject(receipt, CreativeHierarchyTransformStatus::MutationRejected,
           stagedResult.phase, stagedResult.reasonCode, stagedResult.message,
           stagedResult.failedObjectId);
    return receipt;
  }
  if (!stagedResult.changed) {
    receipt.accepted = true;
    receipt.status = CreativeHierarchyTransformStatus::NoChange;
    receipt.phase = CreativeHierarchyTransformPhase::Commit;
    receipt.reasonCode = "creative_hierarchy_transform_no_change";
    receipt.message = receipt.reasonCode;
    return receipt;
  }
  if (!document.commitStagedMutation(std::move(staged))) {
    reject(receipt, CreativeHierarchyTransformStatus::Rejected,
           CreativeHierarchyTransformPhase::Commit,
           "creative_hierarchy_transform_commit_rejected",
           "creative_hierarchy_transform_commit_rejected");
    return receipt;
  }
  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = CreativeHierarchyTransformStatus::Applied;
  receipt.phase = CreativeHierarchyTransformPhase::Commit;
  receipt.revisionAfter = document.revision();
  receipt.reasonCode = "creative_hierarchy_transform_applied";
  receipt.message = receipt.reasonCode;
  return receipt;
}

CreativeHierarchyReattachmentReceipt reattachCreativeObjectHierarchyAtomically(
    CreativeDocument& document,
    const CreativeHierarchyReattachmentRequest& request) {
  CreativeHierarchyReattachmentReceipt receipt =
      makeReattachmentReceipt(document, request);
  if (!document.isValid()) {
    reject(receipt, CreativeHierarchyTransformStatus::InvalidDocument,
           CreativeHierarchyTransformPhase::ValidateRequest,
           "creative_hierarchy_reattachment_document_invalid",
           "creative_hierarchy_reattachment_document_invalid");
    return receipt;
  }
  if (request.expectedDocumentId != document.id() ||
      request.expectedRevision != document.revision()) {
    reject(receipt, CreativeHierarchyTransformStatus::StalePlan,
           CreativeHierarchyTransformPhase::ValidateRequest,
           "creative_hierarchy_reattachment_plan_stale",
           "creative_hierarchy_reattachment_plan_stale");
    return receipt;
  }
  if (request.sourceRootObjectId == kInvalidObjectId ||
      request.targetObjectId == kInvalidObjectId ||
      request.sourceRootObjectId == request.targetObjectId ||
      request.targetSocket.empty() ||
      !isFiniteCreativeVec3(request.targetTransform.position) ||
      !isFiniteCreativeVec3(request.targetTransform.rotationEulerRadians) ||
      !isPositiveCreativeVec3(request.targetTransform.scale)) {
    reject(receipt, CreativeHierarchyTransformStatus::InvalidRequest,
           CreativeHierarchyTransformPhase::ValidateRequest,
           "creative_hierarchy_reattachment_request_invalid",
           "creative_hierarchy_reattachment_request_invalid");
    return receipt;
  }

  const CreativeHierarchySelection hierarchy = resolveCreativeObjectHierarchy(
      document, std::span{&request.sourceRootObjectId, 1U});
  if (!hierarchy.accepted) {
    reject(receipt, hierarchyStatus(hierarchy.status),
           CreativeHierarchyTransformPhase::ResolveHierarchy,
           hierarchy.reasonCode, hierarchy.reasonCode, hierarchy.missingObjectId);
    return receipt;
  }
  receipt.hierarchyObjectCount = hierarchy.objectIds.size();
  if (std::find(hierarchy.objectIds.begin(), hierarchy.objectIds.end(),
                request.targetObjectId) != hierarchy.objectIds.end()) {
    reject(receipt, CreativeHierarchyTransformStatus::TargetInsideSourceHierarchy,
           CreativeHierarchyTransformPhase::ValidateRequest,
           "creative_hierarchy_reattachment_target_inside_source_hierarchy",
           "creative_hierarchy_reattachment_target_inside_source_hierarchy",
           request.targetObjectId);
    return receipt;
  }
  for (CreativeObjectId objectId : hierarchy.objectIds) {
    if (creativeObjectEffectivelyLocked(document, objectId)) {
      reject(receipt, CreativeHierarchyTransformStatus::LockedObject,
             CreativeHierarchyTransformPhase::ValidateLocks,
             "creative_hierarchy_reattachment_source_locked",
             "creative_hierarchy_reattachment_source_locked", objectId);
      return receipt;
    }
  }
  if (creativeObjectEffectivelyLocked(document, request.targetObjectId)) {
    reject(receipt, CreativeHierarchyTransformStatus::LockedObject,
           CreativeHierarchyTransformPhase::ValidateLocks,
           "creative_hierarchy_reattachment_target_locked",
           "creative_hierarchy_reattachment_target_locked",
           request.targetObjectId);
    return receipt;
  }

  const CreativeObject* sourceBefore =
      document.findObject(request.sourceRootObjectId);
  const bool relationshipChanged =
      sourceBefore == nullptr ||
      sourceBefore->parentId !=
          std::optional<CreativeObjectId>{request.targetObjectId} ||
      sourceBefore->attachmentSocket != request.targetSocket;
  CreativeDocument staged = document;
  const StageResult stagedResult = stageAbsoluteHierarchyTransform(
      staged, hierarchy.objectIds, request.sourceRootObjectId,
      request.targetTransform, true, true, true, ExternalParentPolicy::Detach);
  if (!stagedResult.accepted) {
    reject(receipt, CreativeHierarchyTransformStatus::MutationRejected,
           stagedResult.phase, stagedResult.reasonCode, stagedResult.message,
           stagedResult.failedObjectId);
    return receipt;
  }

  const CreativeDocumentMutationReceipt attached = applyDocumentMutation(
      staged, request.sourceRootObjectId, CreativeMutationKind::AttachTo,
      makeAttachPayload(request.targetObjectId, request.targetSocket));
  if (!documentMutationSucceeded(attached.status)) {
    reject(receipt, CreativeHierarchyTransformStatus::MutationRejected,
           CreativeHierarchyTransformPhase::Relationship, attached.message,
           attached.message, request.sourceRootObjectId);
    return receipt;
  }

  receipt.changed = stagedResult.changed || relationshipChanged;
  if (!receipt.changed) {
    receipt.accepted = true;
    receipt.status = CreativeHierarchyTransformStatus::NoChange;
    receipt.phase = CreativeHierarchyTransformPhase::Commit;
    receipt.reasonCode = "creative_hierarchy_reattachment_no_change";
    receipt.message = receipt.reasonCode;
    return receipt;
  }
  if (!document.commitStagedMutation(std::move(staged))) {
    reject(receipt, CreativeHierarchyTransformStatus::Rejected,
           CreativeHierarchyTransformPhase::Commit,
           "creative_hierarchy_reattachment_commit_rejected",
           "creative_hierarchy_reattachment_commit_rejected");
    return receipt;
  }
  receipt.accepted = true;
  receipt.status = CreativeHierarchyTransformStatus::Applied;
  receipt.phase = CreativeHierarchyTransformPhase::Commit;
  receipt.revisionAfter = document.revision();
  receipt.reasonCode = "creative_hierarchy_reattachment_applied";
  receipt.message = receipt.reasonCode;
  return receipt;
}

}  // namespace iggy3d::creative
