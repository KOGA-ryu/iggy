#include "app/iggy3d/creative/tools/Transform.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/tools/Clipboard.hpp"
#include "app/iggy3d/creative/tools/Group.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <unordered_set>
#include <utility>

namespace iggy3d::creative {
namespace {

[[nodiscard]] CreativeVec3 add(CreativeVec3 lhs, CreativeVec3 rhs) noexcept {
  return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

[[nodiscard]] CreativeVec3 subtract(CreativeVec3 lhs,
                                    CreativeVec3 rhs) noexcept {
  return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

[[nodiscard]] CreativeVec3 multiply(CreativeVec3 lhs,
                                    CreativeVec3 rhs) noexcept {
  return {lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z};
}

[[nodiscard]] CreativeVec3 objectAnchor(const CreativeObject& object) noexcept {
  if (!objectHasTransform(object.kind) && objectHasBounds(object.kind)) {
    return object.bounds.min;
  }
  return object.transform.position;
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

[[nodiscard]] CreativeVec3 selectionPivot(
    const std::vector<const CreativeObject*>& objects) noexcept {
  CreativeVec3 pivot{};
  for (const CreativeObject* object : objects) {
    pivot = add(pivot, objectAnchor(*object));
  }
  const double divisor = static_cast<double>(objects.size());
  return {pivot.x / divisor, pivot.y / divisor, pivot.z / divisor};
}

[[nodiscard]] CreativeVec3 rotateAroundYaw(CreativeVec3 value,
                                           CreativeVec3 pivot,
                                           double yawRadians) noexcept {
  const double cosine = std::cos(yawRadians);
  const double sine = std::sin(yawRadians);
  const CreativeVec3 offset = subtract(value, pivot);
  return {pivot.x + offset.x * cosine - offset.z * sine,
          value.y,
          pivot.z + offset.x * sine + offset.z * cosine};
}

[[nodiscard]] CreativeMutationKind requiredMutationKind(
    CreativeTransformCommandKind kind) noexcept {
  switch (kind) {
    case CreativeTransformCommandKind::Translate:
      return CreativeMutationKind::Move;
    case CreativeTransformCommandKind::RotateYaw:
      return CreativeMutationKind::Rotate;
    case CreativeTransformCommandKind::Scale:
      return CreativeMutationKind::Scale;
    case CreativeTransformCommandKind::ResetRotationScale:
      return CreativeMutationKind::Rotate;
  }
  return CreativeMutationKind::Unknown;
}

[[nodiscard]] bool transformOutputValid(
    const CreativeObject& object,
    CreativeVec3 pivot,
    const CreativeTransformCommandRequest& request,
    double yawRadians) noexcept {
  const CreativeVec3 anchor = objectAnchor(object);
  if (!isFiniteCreativeVec3(anchor)) {
    return false;
  }
  if (request.kind == CreativeTransformCommandKind::Translate) {
    return isFiniteCreativeVec3(add(anchor, request.translation));
  }
  if (request.kind == CreativeTransformCommandKind::RotateYaw) {
    CreativeVec3 rotation = object.transform.rotationEulerRadians;
    rotation.y += yawRadians;
    return isFiniteCreativeVec3(
               rotateAroundYaw(anchor, pivot, yawRadians)) &&
           isFiniteCreativeVec3(rotation);
  }
  if (request.kind == CreativeTransformCommandKind::ResetRotationScale) {
    return true;
  }
  return isFiniteCreativeVec3(
             add(pivot, multiply(subtract(anchor, pivot), request.scaleFactor))) &&
         isPositiveCreativeVec3(
             multiply(object.transform.scale, request.scaleFactor));
}

void appendTransformRequests(const CreativeObject& object,
                             CreativeVec3 pivot,
                             const CreativeTransformCommandRequest& request,
                             double yawRadians,
                             std::vector<CreativeMutationRequest>& out) {
  const CreativeVec3 anchor = objectAnchor(object);
  if (request.kind == CreativeTransformCommandKind::Translate) {
    out.push_back({0, object.id, CreativeMutationKind::Move,
                   makeMovePayload(add(anchor, request.translation))});
    return;
  }

  if (request.kind == CreativeTransformCommandKind::RotateYaw) {
    const CreativeVec3 nextPosition =
        rotateAroundYaw(anchor, pivot, yawRadians);
    if (!creativeVec3ExactlyEqual(nextPosition, anchor)) {
      out.push_back({0, object.id, CreativeMutationKind::Move,
                     makeMovePayload(nextPosition)});
    }
    CreativeVec3 rotation = object.transform.rotationEulerRadians;
    rotation.y += yawRadians;
    out.push_back({0, object.id, CreativeMutationKind::Rotate,
                   makeRotatePayload(rotation)});
    return;
  }

  if (request.kind == CreativeTransformCommandKind::ResetRotationScale) {
    out.push_back({0, object.id, CreativeMutationKind::Rotate,
                   makeRotatePayload({0.0, 0.0, 0.0})});
    out.push_back({0, object.id, CreativeMutationKind::Scale,
                   CreativeMutationPayload{
                       ScaleMutation{{1.0, 1.0, 1.0}}}});
    return;
  }

  const CreativeVec3 nextPosition =
      add(pivot, multiply(subtract(anchor, pivot), request.scaleFactor));
  if (!creativeVec3ExactlyEqual(nextPosition, anchor)) {
    out.push_back({0, object.id, CreativeMutationKind::Move,
                   makeMovePayload(nextPosition)});
  }
  out.push_back({0, object.id, CreativeMutationKind::Scale,
                 CreativeMutationPayload{
                     ScaleMutation{multiply(object.transform.scale,
                                            request.scaleFactor)}}});
}

}  // namespace

std::string_view toString(CreativeTransformCommandKind kind) noexcept {
  switch (kind) {
    case CreativeTransformCommandKind::Translate:
      return "Translate";
    case CreativeTransformCommandKind::RotateYaw:
      return "RotateYaw";
    case CreativeTransformCommandKind::Scale:
      return "Scale";
    case CreativeTransformCommandKind::ResetRotationScale:
      return "ResetRotationScale";
  }
  return "Unknown";
}

std::string_view toString(CreativeTransformCommandStatus status) noexcept {
  switch (status) {
    case CreativeTransformCommandStatus::Unknown:
      return "Unknown";
    case CreativeTransformCommandStatus::EmptySelection:
      return "EmptySelection";
    case CreativeTransformCommandStatus::InvalidRequest:
      return "InvalidRequest";
    case CreativeTransformCommandStatus::MissingObject:
      return "MissingObject";
    case CreativeTransformCommandStatus::LockedObject:
      return "LockedObject";
    case CreativeTransformCommandStatus::UnsupportedObject:
      return "UnsupportedObject";
    case CreativeTransformCommandStatus::Applied:
      return "Applied";
    case CreativeTransformCommandStatus::NoChange:
      return "NoChange";
    case CreativeTransformCommandStatus::Rejected:
      return "Rejected";
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
  bool requestValid = false;
  switch (request.kind) {
    case CreativeTransformCommandKind::Translate:
      requestValid = isFiniteCreativeVec3(request.translation);
      break;
    case CreativeTransformCommandKind::RotateYaw:
      requestValid = std::isfinite(request.yawDegrees);
      break;
    case CreativeTransformCommandKind::Scale:
      requestValid = isPositiveCreativeVec3(request.scaleFactor);
      break;
    case CreativeTransformCommandKind::ResetRotationScale:
      requestValid = true;
      break;
  }
  if (!requestValid) {
    receipt.status = CreativeTransformCommandStatus::InvalidRequest;
    receipt.message = "transform_request_invalid";
    return receipt;
  }

  const CreativeHierarchySelection hierarchy =
      resolveCreativeObjectHierarchy(document, objectIds);
  if (!hierarchy.accepted) {
    receipt.failedObjectId = hierarchy.missingObjectId;
    receipt.status = hierarchy.status ==
                             CreativeHierarchySelectionStatus::MissingObject
                         ? CreativeTransformCommandStatus::MissingObject
                         : CreativeTransformCommandStatus::InvalidRequest;
    receipt.message = hierarchy.reasonCode;
    return receipt;
  }
  const ResolvedObjects roots =
      resolveObjectsInDocumentOrder(document, hierarchy.rootObjectIds);
  const ResolvedObjects resolved =
      resolveObjectsInDocumentOrder(document, hierarchy.objectIds);
  if (resolved.missingObjectId != kInvalidObjectId ||
      resolved.objects.empty() || roots.objects.empty()) {
    receipt.failedObjectId = resolved.missingObjectId;
    receipt.status = CreativeTransformCommandStatus::MissingObject;
    receipt.message = "transform_object_missing";
    return receipt;
  }
  receipt.objectCount = resolved.objects.size();
  receipt.pivot = selectionPivot(roots.objects);
  const double yawRadians =
      request.kind == CreativeTransformCommandKind::RotateYaw
          ? request.yawDegrees * std::numbers::pi / 180.0
          : 0.0;

  const CreativeMutationKind requiredKind = requiredMutationKind(request.kind);
  for (const CreativeObject* object : resolved.objects) {
    if (object->locked) {
      receipt.failedObjectId = object->id;
      receipt.failedMutationKind = requiredKind;
      receipt.status = CreativeTransformCommandStatus::LockedObject;
      receipt.message = "transform_object_locked";
      return receipt;
    }
    const bool resetRotationScale =
        request.kind == CreativeTransformCommandKind::ResetRotationScale;
    const bool supportsRequiredMutation =
        resetRotationScale
            ? descriptorAllowsMutation(object->kind,
                                       CreativeMutationKind::Rotate) &&
                  descriptorAllowsMutation(object->kind,
                                           CreativeMutationKind::Scale)
            : descriptorAllowsMutation(object->kind, requiredKind);
    const bool supportsPivotMove =
        request.kind == CreativeTransformCommandKind::Translate ||
        resetRotationScale ||
        descriptorAllowsMutation(object->kind, CreativeMutationKind::Move);
    if (!supportsRequiredMutation || !supportsPivotMove) {
      receipt.failedObjectId = object->id;
      receipt.failedMutationKind =
          resetRotationScale &&
                  descriptorAllowsMutation(object->kind,
                                           CreativeMutationKind::Rotate)
              ? CreativeMutationKind::Scale
              : requiredKind;
      receipt.status = CreativeTransformCommandStatus::UnsupportedObject;
      receipt.message = "transform_object_unsupported";
      return receipt;
    }
    if (!transformOutputValid(*object, receipt.pivot, request, yawRadians)) {
      receipt.failedObjectId = object->id;
      receipt.failedMutationKind = requiredKind;
      receipt.status = CreativeTransformCommandStatus::InvalidRequest;
      receipt.message = "transform_output_invalid";
      return receipt;
    }
  }

  std::vector<CreativeMutationRequest> mutations;
  mutations.reserve(resolved.objects.size() * 2U);
  for (const CreativeObject* object : resolved.objects) {
    appendTransformRequests(*object, receipt.pivot, request, yawRadians,
                            mutations);
  }

  receipt.mutationReceipt =
      applyDocumentMutationsAtomically(document, mutations);
  receipt.revisionAfter = document.revision();
  receipt.accepted = receipt.mutationReceipt.committed &&
                     documentMutationSucceeded(receipt.mutationReceipt.status);
  receipt.changed = receipt.accepted && receipt.mutationReceipt.changed;
  if (receipt.changed) {
    receipt.status = CreativeTransformCommandStatus::Applied;
    receipt.message = "transform_applied";
  } else if (receipt.accepted) {
    receipt.status = CreativeTransformCommandStatus::NoChange;
    receipt.message = "transform_no_change";
  } else {
    receipt.status = CreativeTransformCommandStatus::Rejected;
    receipt.message = "transform_rejected";
    for (const CreativeDocumentMutationReceipt& item :
         receipt.mutationReceipt.receipts) {
      if (documentMutationFailed(item.status)) {
        receipt.failedObjectId = item.objectId;
        receipt.failedMutationKind = item.mutationKind;
        break;
      }
    }
  }
  return receipt;
}

CreativeDuplicateCommandReceipt duplicateDocumentObjectsAtomically(
    CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds,
    const CreativeDuplicateCommandRequest& request) {
  CreativeDuplicateCommandReceipt receipt;
  receipt.requested = true;
  receipt.requestedObjectCount = objectIds.size();
  receipt.revisionBefore = document.revision();
  receipt.revisionAfter = receipt.revisionBefore;

  if (objectIds.empty()) {
    receipt.status = CreativeTransformCommandStatus::EmptySelection;
    receipt.message = "duplicate_selection_empty";
    return receipt;
  }
  if (!isFiniteCreativeVec3(request.offset)) {
    receipt.status = CreativeTransformCommandStatus::InvalidRequest;
    receipt.message = "duplicate_request_invalid";
    return receipt;
  }

  const CreativeHierarchySelection hierarchy =
      resolveCreativeObjectHierarchy(document, objectIds);
  if (!hierarchy.accepted) {
    receipt.failedObjectId = hierarchy.missingObjectId;
    receipt.status = hierarchy.status ==
                             CreativeHierarchySelectionStatus::MissingObject
                         ? CreativeTransformCommandStatus::MissingObject
                         : CreativeTransformCommandStatus::InvalidRequest;
    receipt.message = hierarchy.reasonCode;
    return receipt;
  }

  CreativeClipboard clipboard;
  const CreativeClipboardCopyReceipt copyReceipt =
      copyDocumentObjectsToClipboard(document, hierarchy.objectIds, clipboard);
  if (!copyReceipt.accepted) {
    receipt.failedObjectId = copyReceipt.failedObjectId;
    receipt.status = CreativeTransformCommandStatus::MissingObject;
    receipt.message = "duplicate_object_missing";
    return receipt;
  }

  CreativeClipboardPasteRequest pasteRequest;
  pasteRequest.offset = request.offset;
  pasteRequest.appendCopySuffix = request.appendCopySuffix;
  pasteRequest.externalParentPolicy =
      CreativeClipboardExternalParentPolicy::PreserveIfPresent;
  CreativeClipboardPasteReceipt pasteReceipt =
      pasteCreativeClipboardAtomically(document, clipboard, pasteRequest);
  if (!pasteReceipt.accepted) {
    receipt.failedObjectId = pasteReceipt.failedObjectId;
    receipt.status = pasteReceipt.status ==
                             CreativeClipboardStatus::ObjectIdExhausted
                         ? CreativeTransformCommandStatus::InvalidRequest
                         : CreativeTransformCommandStatus::Rejected;
    receipt.message = pasteReceipt.status ==
                              CreativeClipboardStatus::ObjectIdExhausted
                          ? "duplicate_object_id_exhausted"
                          : pasteReceipt.reasonCode;
    return receipt;
  }

  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = CreativeTransformCommandStatus::Applied;
  receipt.duplicatedObjectIds = std::move(pasteReceipt.pastedObjectIds);
  receipt.duplicatedObjectCount = receipt.duplicatedObjectIds.size();
  receipt.duplicatedSelectionObjectIds.reserve(
      hierarchy.rootObjectIds.size());
  for (CreativeObjectId rootObjectId : hierarchy.rootObjectIds) {
    const auto remap = std::find_if(
        pasteReceipt.idRemaps.begin(), pasteReceipt.idRemaps.end(),
        [rootObjectId](const CreativeClipboardIdRemap& item) {
          return item.sourceObjectId == rootObjectId;
        });
    if (remap != pasteReceipt.idRemaps.end()) {
      receipt.duplicatedSelectionObjectIds.push_back(
          remap->pastedObjectId);
    }
  }
  receipt.revisionAfter = document.revision();
  receipt.message = "duplicate_applied";
  return receipt;
}

}  // namespace iggy3d::creative
