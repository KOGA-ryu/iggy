#include "app/iggy3d/creative/tools/SelectionTransformCommands.hpp"

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/tools/Group.hpp"

#include <cmath>
#include <numbers>
#include <unordered_set>
#include <utility>
#include <vector>

namespace iggy3d::creative {
namespace {

[[nodiscard]] CreativeVec3 add(CreativeVec3 lhs, CreativeVec3 rhs) noexcept {
  return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

struct ResolvedObjects {
  std::vector<const CreativeObject*> objects;
  CreativeObjectId missingObjectId = kInvalidObjectId;
};

[[nodiscard]] ResolvedObjects resolveObjectsInDocumentOrder(
    const CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds) {
  ResolvedObjects resolved;
  std::unordered_set<CreativeObjectId> requested;
  requested.reserve(objectIds.size());
  for (CreativeObjectId objectId : objectIds) {
    if (objectId == kInvalidObjectId) {
      resolved.missingObjectId = objectId;
      return resolved;
    }
    requested.insert(objectId);
  }
  resolved.objects.reserve(requested.size());
  for (const CreativeObject& object : document.objects()) {
    if (requested.contains(object.id)) {
      resolved.objects.push_back(&object);
    }
  }
  if (resolved.objects.size() != requested.size()) {
    for (CreativeObjectId objectId : requested) {
      if (!document.containsObject(objectId)) {
        resolved.missingObjectId = objectId;
        break;
      }
    }
  }
  return resolved;
}

[[nodiscard]] CreativeVec3 objectAnchor(
    const CreativeObject& object) noexcept {
  if (!objectHasTransform(object.kind) && objectHasBounds(object.kind)) {
    return object.bounds.min;
  }
  return object.transform.position;
}

[[nodiscard]] CreativeVec3 selectionPivot(
    const std::vector<const CreativeObject*>& roots) noexcept {
  CreativeVec3 pivot{};
  for (const CreativeObject* object : roots) {
    pivot = add(pivot, objectAnchor(*object));
  }
  const double divisor = static_cast<double>(roots.size());
  return {pivot.x / divisor, pivot.y / divisor, pivot.z / divisor};
}

[[nodiscard]] CreativeTransformCommandStatus transformStatusForPlacement(
    CreativeSelectionPlacementStatus status) noexcept {
  switch (status) {
    case CreativeSelectionPlacementStatus::EmptySource:
      return CreativeTransformCommandStatus::EmptySelection;
    case CreativeSelectionPlacementStatus::InvalidRequest:
    case CreativeSelectionPlacementStatus::InvalidSource:
      return CreativeTransformCommandStatus::InvalidRequest;
    case CreativeSelectionPlacementStatus::MissingObject:
      return CreativeTransformCommandStatus::MissingObject;
    case CreativeSelectionPlacementStatus::LockedObject:
      return CreativeTransformCommandStatus::LockedObject;
    case CreativeSelectionPlacementStatus::UnsupportedObject:
      return CreativeTransformCommandStatus::UnsupportedObject;
    case CreativeSelectionPlacementStatus::Applied:
      return CreativeTransformCommandStatus::Applied;
    case CreativeSelectionPlacementStatus::NoChange:
      return CreativeTransformCommandStatus::NoChange;
    case CreativeSelectionPlacementStatus::Rejected:
      return CreativeTransformCommandStatus::Rejected;
    case CreativeSelectionPlacementStatus::NotRequested:
    case CreativeSelectionPlacementStatus::Planned:
      return CreativeTransformCommandStatus::Unknown;
  }
  return CreativeTransformCommandStatus::Unknown;
}

[[nodiscard]] bool validTransformRequest(
    const CreativeTransformCommandRequest& request) noexcept {
  switch (request.kind) {
    case CreativeTransformCommandKind::Translate:
      return isFiniteCreativeVec3(request.translation);
    case CreativeTransformCommandKind::RotateYaw:
      return std::isfinite(request.yawDegrees);
    case CreativeTransformCommandKind::Scale:
      return isPositiveCreativeVec3(request.scaleFactor);
    case CreativeTransformCommandKind::ResetRotationScale:
      return true;
  }
  return false;
}

void captureFailedMutation(CreativeTransformCommandReceipt& receipt) noexcept {
  for (const CreativeDocumentMutationReceipt& item :
       receipt.mutationReceipt.receipts) {
    if (documentMutationFailed(item.status)) {
      receipt.failedObjectId = item.objectId;
      receipt.failedMutationKind = item.mutationKind;
      return;
    }
  }
}

}  // namespace

std::string_view toString(CreativeTransformCommandKind kind) noexcept {
  switch (kind) {
    case CreativeTransformCommandKind::Translate: return "Translate";
    case CreativeTransformCommandKind::RotateYaw: return "RotateYaw";
    case CreativeTransformCommandKind::Scale: return "Scale";
    case CreativeTransformCommandKind::ResetRotationScale:
      return "ResetRotationScale";
  }
  return "Unknown";
}

std::string_view toString(CreativeTransformCommandStatus status) noexcept {
  switch (status) {
    case CreativeTransformCommandStatus::Unknown: return "Unknown";
    case CreativeTransformCommandStatus::EmptySelection:
      return "EmptySelection";
    case CreativeTransformCommandStatus::InvalidRequest:
      return "InvalidRequest";
    case CreativeTransformCommandStatus::MissingObject: return "MissingObject";
    case CreativeTransformCommandStatus::LockedObject: return "LockedObject";
    case CreativeTransformCommandStatus::UnsupportedObject:
      return "UnsupportedObject";
    case CreativeTransformCommandStatus::Applied: return "Applied";
    case CreativeTransformCommandStatus::NoChange: return "NoChange";
    case CreativeTransformCommandStatus::Rejected: return "Rejected";
  }
  return "Unknown";
}

CreativeTransformCommandReceipt transformDocumentObjectsAtomically(
    CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds,
    const CreativeTransformCommandRequest& request) {
  CreativeTransformCommandReceipt receipt;
  receipt.requested = true;
  receipt.kind = request.kind;
  receipt.requestedObjectCount = objectIds.size();
  receipt.revisionBefore = document.revision();
  receipt.revisionAfter = receipt.revisionBefore;
  if (objectIds.empty()) {
    receipt.status = CreativeTransformCommandStatus::EmptySelection;
    receipt.message = "transform_selection_empty";
    return receipt;
  }
  if (!validTransformRequest(request)) {
    receipt.status = CreativeTransformCommandStatus::InvalidRequest;
    receipt.message = "transform_request_invalid";
    return receipt;
  }

  const CreativeHierarchySelection hierarchy =
      resolveCreativeObjectHierarchy(document, objectIds);
  if (!hierarchy.accepted) {
    receipt.failedObjectId = hierarchy.missingObjectId;
    receipt.status =
        hierarchy.status == CreativeHierarchySelectionStatus::MissingObject
            ? CreativeTransformCommandStatus::MissingObject
            : CreativeTransformCommandStatus::InvalidRequest;
    receipt.message = hierarchy.reasonCode;
    return receipt;
  }
  const ResolvedObjects roots =
      resolveObjectsInDocumentOrder(document, hierarchy.rootObjectIds);
  const ResolvedObjects resolved =
      resolveObjectsInDocumentOrder(document, hierarchy.objectIds);
  if (roots.objects.empty() || resolved.objects.empty() ||
      roots.missingObjectId != kInvalidObjectId ||
      resolved.missingObjectId != kInvalidObjectId) {
    receipt.failedObjectId = resolved.missingObjectId != kInvalidObjectId
                                 ? resolved.missingObjectId
                                 : roots.missingObjectId;
    receipt.status = CreativeTransformCommandStatus::MissingObject;
    receipt.message = "transform_object_missing";
    return receipt;
  }
  receipt.objectCount = resolved.objects.size();
  receipt.pivot = selectionPivot(roots.objects);

  if (request.kind == CreativeTransformCommandKind::ResetRotationScale) {
    std::vector<CreativeMutationRequest> mutations;
    mutations.reserve(resolved.objects.size() * 2U);
    for (const CreativeObject* object : resolved.objects) {
      if (creativeObjectEffectivelyLocked(document, object->id)) {
        receipt.failedObjectId = object->id;
        receipt.failedMutationKind = CreativeMutationKind::Rotate;
        receipt.status = CreativeTransformCommandStatus::LockedObject;
        receipt.message = "transform_object_locked";
        return receipt;
      }
      const bool rotates = descriptorAllowsMutation(
          object->kind, CreativeMutationKind::Rotate);
      const bool scales = descriptorAllowsMutation(
          object->kind, CreativeMutationKind::Scale);
      if (!rotates || !scales) {
        receipt.failedObjectId = object->id;
        receipt.failedMutationKind = rotates ? CreativeMutationKind::Scale
                                             : CreativeMutationKind::Rotate;
        receipt.status = CreativeTransformCommandStatus::UnsupportedObject;
        receipt.message = "transform_object_unsupported";
        return receipt;
      }
      mutations.push_back(
          {0, object->id, CreativeMutationKind::Rotate,
           makeRotatePayload({0.0, 0.0, 0.0})});
      mutations.push_back(
          {0, object->id, CreativeMutationKind::Scale,
           CreativeMutationPayload{ScaleMutation{{1.0, 1.0, 1.0}}}});
    }
    receipt.mutationReceipt =
        applyDocumentMutationsAtomically(document, mutations);
    receipt.revisionAfter = document.revision();
    receipt.accepted = receipt.mutationReceipt.committed &&
                       documentMutationSucceeded(
                           receipt.mutationReceipt.status);
    receipt.changed = receipt.accepted && receipt.mutationReceipt.changed;
    receipt.status = !receipt.accepted
                         ? CreativeTransformCommandStatus::Rejected
                         : receipt.changed
                               ? CreativeTransformCommandStatus::Applied
                               : CreativeTransformCommandStatus::NoChange;
    receipt.message = !receipt.accepted
                          ? "transform_rejected"
                          : receipt.changed ? "transform_applied"
                                            : "transform_no_change";
    if (!receipt.accepted) {
      captureFailedMutation(receipt);
    }
    return receipt;
  }

  CreativeSelectionPlacementRequest placement;
  placement.mode = CreativeSelectionPlacementMode::Move;
  placement.sourceAnchor = receipt.pivot;
  placement.targetAnchor = receipt.pivot;
  switch (request.kind) {
    case CreativeTransformCommandKind::Translate:
      placement.targetAnchor = add(receipt.pivot, request.translation);
      break;
    case CreativeTransformCommandKind::RotateYaw:
      placement.hasAxisAngleRotation = request.yawDegrees != 0.0;
      placement.rotationAxis = CreativeAxis3::Y;
      placement.rotationRadians =
          request.yawDegrees * std::numbers::pi / 180.0;
      break;
    case CreativeTransformCommandKind::Scale:
      placement.scaleFactor = request.scaleFactor;
      break;
    case CreativeTransformCommandKind::ResetRotationScale:
      break;
  }

  CreativeSelectionPlacementReceipt placed =
      placeDocumentObjectsAtomically(document, hierarchy.objectIds, placement);
  receipt.accepted = placed.accepted;
  receipt.changed = placed.changed;
  receipt.status = transformStatusForPlacement(placed.status);
  receipt.objectCount = placed.objectCount;
  receipt.failedObjectId = placed.failedObjectId;
  receipt.failedMutationKind = placed.failedMutationKind;
  receipt.revisionAfter = placed.revisionAfter;
  receipt.mutationReceipt = std::move(placed.mutationReceipt);
  receipt.message = receipt.changed
                        ? "transform_applied"
                        : receipt.accepted ? "transform_no_change"
                                           : placed.reasonCode;
  return receipt;
}

}  // namespace iggy3d::creative
