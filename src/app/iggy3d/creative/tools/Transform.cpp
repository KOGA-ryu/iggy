#include "app/iggy3d/creative/tools/Transform.hpp"

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace iggy3d::creative {
namespace {

[[nodiscard]] bool finiteVec3(CreativeVec3 value) noexcept {
  return std::isfinite(value.x) && std::isfinite(value.y) &&
         std::isfinite(value.z);
}

[[nodiscard]] bool positiveVec3(CreativeVec3 value) noexcept {
  return finiteVec3(value) && value.x > 0.0 && value.y > 0.0 &&
         value.z > 0.0;
}

[[nodiscard]] bool sameVec3(CreativeVec3 lhs, CreativeVec3 rhs) noexcept {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

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
                                           double yawDegrees) noexcept {
  const double radians = yawDegrees * std::numbers::pi / 180.0;
  const double cosine = std::cos(radians);
  const double sine = std::sin(radians);
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
  }
  return CreativeMutationKind::Unknown;
}

[[nodiscard]] bool transformOutputValid(
    const CreativeObject& object,
    CreativeVec3 pivot,
    const CreativeTransformCommandRequest& request) noexcept {
  const CreativeVec3 anchor = objectAnchor(object);
  if (!finiteVec3(anchor)) {
    return false;
  }
  if (request.kind == CreativeTransformCommandKind::Translate) {
    return finiteVec3(add(anchor, request.translation));
  }
  if (request.kind == CreativeTransformCommandKind::RotateYaw) {
    CreativeVec3 rotation = object.transform.rotation;
    rotation.y += request.yawDegrees;
    return finiteVec3(rotateAroundYaw(anchor, pivot, request.yawDegrees)) &&
           finiteVec3(rotation);
  }
  return finiteVec3(
             add(pivot, multiply(subtract(anchor, pivot), request.scaleFactor))) &&
         positiveVec3(multiply(object.transform.scale, request.scaleFactor));
}

void appendTransformRequests(const CreativeObject& object,
                             CreativeVec3 pivot,
                             const CreativeTransformCommandRequest& request,
                             std::vector<CreativeMutationRequest>& out) {
  const CreativeVec3 anchor = objectAnchor(object);
  if (request.kind == CreativeTransformCommandKind::Translate) {
    out.push_back({0, object.id, CreativeMutationKind::Move,
                   makeMovePayload(add(anchor, request.translation))});
    return;
  }

  if (request.kind == CreativeTransformCommandKind::RotateYaw) {
    const CreativeVec3 nextPosition =
        rotateAroundYaw(anchor, pivot, request.yawDegrees);
    if (!sameVec3(nextPosition, anchor)) {
      out.push_back({0, object.id, CreativeMutationKind::Move,
                     makeMovePayload(nextPosition)});
    }
    CreativeVec3 rotation = object.transform.rotation;
    rotation.y += request.yawDegrees;
    out.push_back({0, object.id, CreativeMutationKind::Rotate,
                   makeRotatePayload(rotation)});
    return;
  }

  const CreativeVec3 nextPosition =
      add(pivot, multiply(subtract(anchor, pivot), request.scaleFactor));
  if (!sameVec3(nextPosition, anchor)) {
    out.push_back({0, object.id, CreativeMutationKind::Move,
                   makeMovePayload(nextPosition)});
  }
  out.push_back({0, object.id, CreativeMutationKind::Scale,
                 CreativeMutationPayload{
                     ScaleMutation{multiply(object.transform.scale,
                                            request.scaleFactor)}}});
}

[[nodiscard]] CreativeDocumentCreateRequest duplicateRequestForObject(
    const CreativeObject& object,
    const CreativeDuplicateCommandRequest& request,
    std::optional<CreativeObjectId> parentId) {
  const CreativeObjectDescriptor& descriptor = describeObject(object.kind);
  CreativeDocumentCreateRequest duplicate;
  duplicate.kind = object.kind;
  duplicate.name = request.appendCopySuffix ? object.name + " Copy" : object.name;
  duplicate.transform = object.transform;
  duplicate.hasTransformOverride = descriptor.hasTransform;
  if (duplicate.hasTransformOverride) {
    duplicate.transform.position = add(duplicate.transform.position, request.offset);
  }
  duplicate.bounds = object.bounds;
  duplicate.hasBoundsOverride = descriptor.hasBounds;
  if (duplicate.hasBoundsOverride) {
    duplicate.bounds.min = add(duplicate.bounds.min, request.offset);
    duplicate.bounds.max = add(duplicate.bounds.max, request.offset);
  }
  duplicate.layerId = object.layerId;
  duplicate.hasLayerOverride = true;
  duplicate.visible = object.visible;
  duplicate.hasVisibleOverride = true;
  duplicate.locked = object.locked;
  duplicate.hasLockedOverride = true;
  duplicate.tags = object.tags;
  duplicate.parentId = parentId;
  duplicate.pathPoints = object.pathPoints;
  duplicate.hasPathOverride = !object.pathPoints.empty();
  for (CreativePathPoint& point : duplicate.pathPoints) {
    point.position = add(point.position, request.offset);
  }
  return duplicate;
}

[[nodiscard]] bool validDuplicateRequest(
    const CreativeDocumentCreateRequest& request) noexcept {
  if (request.hasTransformOverride &&
      (!finiteVec3(request.transform.position) ||
       !finiteVec3(request.transform.rotation) ||
       !positiveVec3(request.transform.scale))) {
    return false;
  }
  if (request.hasBoundsOverride &&
      (!finiteVec3(request.bounds.min) || !finiteVec3(request.bounds.max))) {
    return false;
  }
  return std::all_of(
      request.pathPoints.begin(), request.pathPoints.end(),
      [](const CreativePathPoint& point) { return finiteVec3(point.position); });
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
  const bool requestValid =
      request.kind == CreativeTransformCommandKind::Translate
          ? finiteVec3(request.translation)
          : request.kind == CreativeTransformCommandKind::RotateYaw
                ? std::isfinite(request.yawDegrees)
                : positiveVec3(request.scaleFactor);
  if (!requestValid) {
    receipt.status = CreativeTransformCommandStatus::InvalidRequest;
    receipt.message = "transform_request_invalid";
    return receipt;
  }

  const ResolvedObjects resolved =
      resolveObjectsInDocumentOrder(document, objectIds);
  if (resolved.missingObjectId != kInvalidObjectId ||
      resolved.objects.empty()) {
    receipt.failedObjectId = resolved.missingObjectId;
    receipt.status = CreativeTransformCommandStatus::MissingObject;
    receipt.message = "transform_object_missing";
    return receipt;
  }
  receipt.objectCount = resolved.objects.size();
  receipt.pivot = selectionPivot(resolved.objects);

  const CreativeMutationKind requiredKind = requiredMutationKind(request.kind);
  for (const CreativeObject* object : resolved.objects) {
    if (object->locked) {
      receipt.failedObjectId = object->id;
      receipt.failedMutationKind = requiredKind;
      receipt.status = CreativeTransformCommandStatus::LockedObject;
      receipt.message = "transform_object_locked";
      return receipt;
    }
    if (!descriptorAllowsMutation(object->kind, requiredKind) ||
        (request.kind != CreativeTransformCommandKind::Translate &&
         !descriptorAllowsMutation(object->kind, CreativeMutationKind::Move))) {
      receipt.failedObjectId = object->id;
      receipt.failedMutationKind = requiredKind;
      receipt.status = CreativeTransformCommandStatus::UnsupportedObject;
      receipt.message = "transform_object_unsupported";
      return receipt;
    }
    if (!transformOutputValid(*object, receipt.pivot, request)) {
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
    appendTransformRequests(*object, receipt.pivot, request, mutations);
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
  if (!finiteVec3(request.offset)) {
    receipt.status = CreativeTransformCommandStatus::InvalidRequest;
    receipt.message = "duplicate_request_invalid";
    return receipt;
  }

  const ResolvedObjects resolved =
      resolveObjectsInDocumentOrder(document, objectIds);
  if (resolved.missingObjectId != kInvalidObjectId ||
      resolved.objects.empty()) {
    receipt.failedObjectId = resolved.missingObjectId;
    receipt.status = CreativeTransformCommandStatus::MissingObject;
    receipt.message = "duplicate_object_missing";
    return receipt;
  }

  const CreativeObjectId nextObjectId = document.nextObjectId();
  const CreativeObjectId remainingIds =
      std::numeric_limits<CreativeObjectId>::max() - nextObjectId;
  if (nextObjectId == kInvalidObjectId ||
      resolved.objects.size() > remainingIds) {
    receipt.status = CreativeTransformCommandStatus::InvalidRequest;
    receipt.message = "duplicate_object_id_exhausted";
    return receipt;
  }

  CreativeDocument stagedDocument = document;
  std::unordered_map<CreativeObjectId, CreativeObjectId> duplicateIds;
  duplicateIds.reserve(resolved.objects.size());
  receipt.duplicatedObjectIds.reserve(resolved.objects.size());
  for (const CreativeObject* object : resolved.objects) {
    std::optional<CreativeObjectId> parentId = object->parentId;
    if (parentId.has_value()) {
      const auto duplicateParent = duplicateIds.find(*parentId);
      if (duplicateParent != duplicateIds.end()) {
        parentId = duplicateParent->second;
      }
    }
    CreativeDocumentCreateRequest duplicateRequest =
        duplicateRequestForObject(*object, request, parentId);
    if (!validDuplicateRequest(duplicateRequest)) {
      receipt.failedObjectId = object->id;
      receipt.status = CreativeTransformCommandStatus::InvalidRequest;
      receipt.message = "duplicate_output_invalid";
      return receipt;
    }
    const CreativeDocumentCreateReceipt createReceipt =
        stagedDocument.createObject(duplicateRequest);
    if (!createReceipt.accepted || !createReceipt.objectCreated ||
        !createReceipt.changed) {
      receipt.failedObjectId = object->id;
      receipt.status = CreativeTransformCommandStatus::Rejected;
      receipt.message = std::string(createReceipt.reasonCode);
      return receipt;
    }
    duplicateIds.emplace(object->id, createReceipt.objectId);
    receipt.duplicatedObjectIds.push_back(createReceipt.objectId);
  }

  document = std::move(stagedDocument);
  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = CreativeTransformCommandStatus::Applied;
  receipt.duplicatedObjectCount = receipt.duplicatedObjectIds.size();
  receipt.revisionAfter = document.revision();
  receipt.message = "duplicate_applied";
  return receipt;
}

}  // namespace iggy3d::creative
