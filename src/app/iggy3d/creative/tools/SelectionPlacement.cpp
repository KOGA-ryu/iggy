#include "app/iggy3d/creative/tools/SelectionPlacement.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "core/math/Snap.hpp"

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

[[nodiscard]] bool samePathPoints(
    std::span<const CreativePathPoint> lhs,
    std::span<const CreativePathPoint> rhs) noexcept {
  return lhs.size() == rhs.size() &&
         std::equal(lhs.begin(), lhs.end(), rhs.begin(),
                    [](const CreativePathPoint& left,
                       const CreativePathPoint& right) {
                      return creativeVec3ExactlyEqual(left.position,
                                                      right.position);
                    });
}

[[nodiscard]] CreativeBounds translateBounds(CreativeBounds bounds,
                                              CreativeVec3 delta) noexcept {
  bounds.min = add(bounds.min, delta);
  bounds.max = add(bounds.max, delta);
  return bounds;
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

[[nodiscard]] bool validPlacementMode(
    CreativeSelectionPlacementMode mode) noexcept {
  return mode == CreativeSelectionPlacementMode::Copy ||
         mode == CreativeSelectionPlacementMode::Move;
}

[[nodiscard]] bool validPlacementAxis(
    CreativeSelectionPlacementAxis axis) noexcept {
  return static_cast<std::size_t>(axis) <
         static_cast<std::size_t>(CreativeSelectionPlacementAxis::Count);
}

[[nodiscard]] CreativeVec3 constrainedDisplacement(
    CreativeVec3 displacement,
    CreativeSelectionPlacementAxis axis,
    double snapStepMeters) noexcept {
  switch (axis) {
    case CreativeSelectionPlacementAxis::Free:
      return displacement;
    case CreativeSelectionPlacementAxis::X:
      return {iggy3d::snapScalarToGrid(displacement.x, snapStepMeters, 0.0),
              0.0, 0.0};
    case CreativeSelectionPlacementAxis::Y:
      return {0.0,
              iggy3d::snapScalarToGrid(displacement.y, snapStepMeters, 0.0),
              0.0};
    case CreativeSelectionPlacementAxis::Z:
      return {0.0, 0.0,
              iggy3d::snapScalarToGrid(displacement.z, snapStepMeters, 0.0)};
    case CreativeSelectionPlacementAxis::Count:
      return {};
  }
  return {};
}

void addAxisNudge(CreativeVec3& displacement,
                  CreativeVec3 nudgeOffset,
                  CreativeSelectionPlacementAxis axis) noexcept {
  switch (axis) {
    case CreativeSelectionPlacementAxis::Free:
      displacement = add(displacement, nudgeOffset);
      return;
    case CreativeSelectionPlacementAxis::X:
      displacement.x += nudgeOffset.x;
      return;
    case CreativeSelectionPlacementAxis::Y:
      displacement.y += nudgeOffset.y;
      return;
    case CreativeSelectionPlacementAxis::Z:
      displacement.z += nudgeOffset.z;
      return;
    case CreativeSelectionPlacementAxis::Count:
      return;
  }
}

void addNudgeStep(CreativeVec3& offset,
                  CreativeSelectionPlacementAxis axis,
                  double delta) noexcept {
  switch (axis) {
    case CreativeSelectionPlacementAxis::X:
      offset.x += delta;
      return;
    case CreativeSelectionPlacementAxis::Y:
      offset.y += delta;
      return;
    case CreativeSelectionPlacementAxis::Z:
      offset.z += delta;
      return;
    case CreativeSelectionPlacementAxis::Free:
    case CreativeSelectionPlacementAxis::Count:
      return;
  }
}

[[nodiscard]] CreativeVec3 transformPlacementOffset(
    CreativeVec3 offset,
    const CreativeSelectionPlacementRequest& request) noexcept {
  if (request.mirrorX) {
    offset.x = -offset.x;
  }
  if (request.mirrorZ) {
    offset.z = -offset.z;
  }
  switch (request.quarterTurns) {
    case 0U: return offset;
    case 1U: return {offset.z, offset.y, -offset.x};
    case 2U: return {-offset.x, offset.y, -offset.z};
    case 3U: return {-offset.z, offset.y, offset.x};
    default: return {};
  }
}

[[nodiscard]] CreativeVec3 transformPlacementPoint(
    CreativeVec3 point,
    const CreativeSelectionPlacementRequest& request) noexcept {
  return add(request.targetAnchor,
             transformPlacementOffset(subtract(point, request.sourceAnchor),
                                      request));
}

[[nodiscard]] double transformPlacementYaw(
    double yawRadians,
    const CreativeSelectionPlacementRequest& request) noexcept {
  if (request.quarterTurns == 0U && !request.mirrorX && !request.mirrorZ) {
    return yawRadians;
  }
  double transformed = yawRadians;
  if (request.mirrorX) {
    transformed = -transformed;
  }
  if (request.mirrorZ) {
    transformed = std::numbers::pi - transformed;
  }
  transformed += static_cast<double>(request.quarterTurns) *
                 std::numbers::pi * 0.5;
  return std::remainder(transformed, std::numbers::pi * 2.0);
}

[[nodiscard]] CreativeBounds transformPlacementBounds(
    CreativeBounds bounds,
    const CreativeSelectionPlacementRequest& request) noexcept {
  CreativeBounds output{};
  bool initialized = false;
  for (std::size_t index = 0; index < 8U; ++index) {
    const CreativeVec3 corner{
        (index & 1U) != 0U ? bounds.max.x : bounds.min.x,
        (index & 2U) != 0U ? bounds.max.y : bounds.min.y,
        (index & 4U) != 0U ? bounds.max.z : bounds.min.z,
    };
    const CreativeVec3 transformed = transformPlacementPoint(corner, request);
    if (!initialized) {
      output = {transformed, transformed};
      initialized = true;
      continue;
    }
    output.min.x = std::min(output.min.x, transformed.x);
    output.min.y = std::min(output.min.y, transformed.y);
    output.min.z = std::min(output.min.z, transformed.z);
    output.max.x = std::max(output.max.x, transformed.x);
    output.max.y = std::max(output.max.y, transformed.y);
    output.max.z = std::max(output.max.z, transformed.z);
  }
  return output;
}

[[nodiscard]] bool validPlacementObject(const CreativeObject& object) noexcept {
  return object.id != kInvalidObjectId &&
         object.kind != CreativeObjectKind::Unknown &&
         object.kind != CreativeObjectKind::Count &&
         isFiniteCreativeVec3(object.transform.position) &&
         isFiniteCreativeVec3(object.transform.rotationEulerRadians) &&
         isPositiveCreativeVec3(object.transform.scale) &&
         isFiniteCreativeVec3(object.bounds.min) &&
         isFiniteCreativeVec3(object.bounds.max) &&
         std::all_of(object.pathPoints.begin(), object.pathPoints.end(),
                     [](const CreativePathPoint& point) {
                       return isFiniteCreativeVec3(point.position);
                     });
}

[[nodiscard]] CreativeObject transformPlacementObject(
    const CreativeObject& source,
    const CreativeSelectionPlacementRequest& request) {
  CreativeObject output = source;
  if (objectHasTransform(source.kind)) {
    output.transform.position =
        transformPlacementPoint(source.transform.position, request);
    output.transform.rotationEulerRadians.y = transformPlacementYaw(
        source.transform.rotationEulerRadians.y, request);
    if (objectHasBounds(source.kind)) {
      output.bounds = translateBounds(
          source.bounds,
          subtract(output.transform.position, source.transform.position));
    }
  } else if (objectHasBounds(source.kind)) {
    output.bounds = transformPlacementBounds(source.bounds, request);
  }
  for (CreativePathPoint& point : output.pathPoints) {
    point.position = transformPlacementPoint(point.position, request);
  }
  return output;
}

void includePlacementPoint(CreativeSelectionPlacementPlan& plan,
                           CreativeVec3 point) noexcept {
  if (!plan.hasAggregateBounds) {
    plan.aggregateBounds = {point, point};
    plan.hasAggregateBounds = true;
    return;
  }
  plan.aggregateBounds.min.x = std::min(plan.aggregateBounds.min.x, point.x);
  plan.aggregateBounds.min.y = std::min(plan.aggregateBounds.min.y, point.y);
  plan.aggregateBounds.min.z = std::min(plan.aggregateBounds.min.z, point.z);
  plan.aggregateBounds.max.x = std::max(plan.aggregateBounds.max.x, point.x);
  plan.aggregateBounds.max.y = std::max(plan.aggregateBounds.max.y, point.y);
  plan.aggregateBounds.max.z = std::max(plan.aggregateBounds.max.z, point.z);
}

[[nodiscard]] bool includePlacementObject(
    CreativeSelectionPlacementPlan& plan,
    const CreativeObject& object) noexcept {
  if (objectHasBounds(object.kind)) {
    const CreativeTransformedBounds resolved =
        resolveCreativeObjectBounds(object);
    if (!resolved.valid) {
      return false;
    }
    includePlacementPoint(plan, resolved.worldBounds.min);
    includePlacementPoint(plan, resolved.worldBounds.max);
  } else if (objectHasTransform(object.kind)) {
    includePlacementPoint(plan, object.transform.position);
  }
  for (const CreativePathPoint& point : object.pathPoints) {
    includePlacementPoint(plan, point.position);
  }
  return plan.hasAggregateBounds;
}

[[nodiscard]] CreativeMutationKind unsupportedPlacementMutation(
    const CreativeObject& source,
    const CreativeObject& transformed) noexcept {
  if (objectHasTransform(source.kind) &&
      !creativeVec3ExactlyEqual(source.transform.position,
                                transformed.transform.position) &&
      !descriptorAllowsMutation(source.kind, CreativeMutationKind::Move)) {
    return CreativeMutationKind::Move;
  }
  if (objectHasTransform(source.kind) &&
      !creativeVec3ExactlyEqual(source.transform.rotationEulerRadians,
                                transformed.transform.rotationEulerRadians) &&
      !descriptorAllowsMutation(source.kind, CreativeMutationKind::Rotate)) {
    return CreativeMutationKind::Rotate;
  }
  if (!objectHasTransform(source.kind) && objectHasBounds(source.kind) &&
      !creativeBoundsExactlyEqual(source.bounds, transformed.bounds) &&
      !descriptorAllowsMutation(source.kind, CreativeMutationKind::SetBounds)) {
    return CreativeMutationKind::SetBounds;
  }
  if (!samePathPoints(source.pathPoints, transformed.pathPoints) &&
      !descriptorAllowsMutation(source.kind,
                                CreativeMutationKind::SetPatrolRoute)) {
    return CreativeMutationKind::SetPatrolRoute;
  }
  return CreativeMutationKind::Unknown;
}

void appendPlacementMutations(const CreativeObject& source,
                              const CreativeObject& transformed,
                              std::vector<CreativeMutationRequest>& mutations) {
  if (objectHasTransform(source.kind) &&
      !creativeVec3ExactlyEqual(source.transform.position,
                                transformed.transform.position)) {
    mutations.push_back(
        {0, source.id, CreativeMutationKind::Move,
         makeMovePayload(transformed.transform.position)});
  }
  if (objectHasTransform(source.kind) &&
      !creativeVec3ExactlyEqual(source.transform.rotationEulerRadians,
                                transformed.transform.rotationEulerRadians)) {
    mutations.push_back(
        {0, source.id, CreativeMutationKind::Rotate,
         makeRotatePayload(transformed.transform.rotationEulerRadians)});
  }
  if (!objectHasTransform(source.kind) && objectHasBounds(source.kind) &&
      !creativeBoundsExactlyEqual(source.bounds, transformed.bounds)) {
    mutations.push_back(
        {0, source.id, CreativeMutationKind::SetBounds,
         makeBoundsPayload(transformed.bounds)});
  }
  if (!samePathPoints(source.pathPoints, transformed.pathPoints)) {
    mutations.push_back(
        {0, source.id, CreativeMutationKind::SetPatrolRoute,
         makePathPointsPayload(transformed.pathPoints)});
  }
}

}  // namespace

std::string_view toString(CreativeSelectionPlacementMode mode) noexcept {
  switch (mode) {
    case CreativeSelectionPlacementMode::Copy: return "Copy";
    case CreativeSelectionPlacementMode::Move: return "Move";
  }
  return "Unknown";
}

std::string_view toString(CreativeSelectionPlacementAxis axis) noexcept {
  switch (axis) {
    case CreativeSelectionPlacementAxis::Free: return "FREE";
    case CreativeSelectionPlacementAxis::X: return "X";
    case CreativeSelectionPlacementAxis::Y: return "Y";
    case CreativeSelectionPlacementAxis::Z: return "Z";
    case CreativeSelectionPlacementAxis::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeSelectionPlacementStatus status) noexcept {
  switch (status) {
    case CreativeSelectionPlacementStatus::NotRequested: return "NotRequested";
    case CreativeSelectionPlacementStatus::EmptySource: return "EmptySource";
    case CreativeSelectionPlacementStatus::InvalidRequest:
      return "InvalidRequest";
    case CreativeSelectionPlacementStatus::InvalidSource: return "InvalidSource";
    case CreativeSelectionPlacementStatus::MissingObject: return "MissingObject";
    case CreativeSelectionPlacementStatus::LockedObject: return "LockedObject";
    case CreativeSelectionPlacementStatus::UnsupportedObject:
      return "UnsupportedObject";
    case CreativeSelectionPlacementStatus::Planned: return "Planned";
    case CreativeSelectionPlacementStatus::Applied: return "Applied";
    case CreativeSelectionPlacementStatus::NoChange: return "NoChange";
    case CreativeSelectionPlacementStatus::Rejected: return "Rejected";
  }
  return "Unknown";
}

CreativeSelectionPlacementTargetResult
resolveCreativeSelectionPlacementTarget(
    const CreativeSelectionPlacementTargetRequest& request) noexcept {
  CreativeSelectionPlacementTargetResult result;
  result.requested = true;
  result.request = request;
  if (!validPlacementAxis(request.axis) ||
      !isFiniteCreativeVec3(request.sourceAnchor) ||
      !isFiniteCreativeVec3(request.aimedAnchor) ||
      !isFiniteCreativeVec3(request.nudgeOffset) ||
      !std::isfinite(request.snapStepMeters) ||
      request.snapStepMeters <= 0.0) {
    result.status = CreativeSelectionPlacementTargetStatus::InvalidRequest;
    result.reasonCode = "selection_placement_target_invalid";
    return result;
  }

  result.displacement = constrainedDisplacement(
      subtract(request.aimedAnchor, request.sourceAnchor), request.axis,
      request.snapStepMeters);
  addAxisNudge(result.displacement, request.nudgeOffset, request.axis);
  result.targetAnchor = add(request.sourceAnchor, result.displacement);
  if (!isFiniteCreativeVec3(result.displacement) ||
      !isFiniteCreativeVec3(result.targetAnchor)) {
    result.displacement = {};
    result.targetAnchor = {};
    result.status = CreativeSelectionPlacementTargetStatus::InvalidRequest;
    result.reasonCode = "selection_placement_target_overflow";
    return result;
  }

  result.accepted = true;
  result.status = CreativeSelectionPlacementTargetStatus::Resolved;
  result.reasonCode = "selection_placement_target_resolved";
  return result;
}

CreativeSelectionPlacementNudgeReceipt
nudgeCreativeSelectionPlacementOffset(
    const CreativeSelectionPlacementNudgeRequest& request) noexcept {
  CreativeSelectionPlacementNudgeReceipt receipt;
  receipt.requested = true;
  receipt.request = request;
  receipt.offset = request.offset;
  if (!validPlacementAxis(request.axis) ||
      !isFiniteCreativeVec3(request.offset) ||
      !std::isfinite(request.snapStepMeters) ||
      request.snapStepMeters <= 0.0) {
    receipt.status = CreativeSelectionPlacementNudgeStatus::InvalidRequest;
    receipt.reasonCode = "selection_placement_nudge_invalid";
    return receipt;
  }
  if (request.axis == CreativeSelectionPlacementAxis::Free) {
    receipt.status = CreativeSelectionPlacementNudgeStatus::AxisRequired;
    receipt.reasonCode = "selection_placement_nudge_axis_required";
    return receipt;
  }
  receipt.appliedStepMeters =
      request.snapStepMeters * (request.fine ? 0.25 : 1.0);
  if (!std::isfinite(receipt.appliedStepMeters) ||
      receipt.appliedStepMeters <= 0.0) {
    receipt.appliedStepMeters = 0.0;
    receipt.status = CreativeSelectionPlacementNudgeStatus::InvalidRequest;
    receipt.reasonCode = "selection_placement_nudge_step_invalid";
    return receipt;
  }
  if (request.steps == 0) {
    receipt.accepted = true;
    receipt.status = CreativeSelectionPlacementNudgeStatus::NoChange;
    receipt.reasonCode = "selection_placement_nudge_no_change";
    return receipt;
  }

  const double delta = receipt.appliedStepMeters *
                       static_cast<double>(request.steps);
  if (!std::isfinite(delta)) {
    receipt.status = CreativeSelectionPlacementNudgeStatus::InvalidRequest;
    receipt.reasonCode = "selection_placement_nudge_overflow";
    return receipt;
  }
  addNudgeStep(receipt.offset, request.axis, delta);
  if (!isFiniteCreativeVec3(receipt.offset)) {
    receipt.offset = request.offset;
    receipt.status = CreativeSelectionPlacementNudgeStatus::InvalidRequest;
    receipt.reasonCode = "selection_placement_nudge_overflow";
    return receipt;
  }

  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = CreativeSelectionPlacementNudgeStatus::Applied;
  receipt.reasonCode = "selection_placement_nudge_applied";
  return receipt;
}

CreativeSelectionPlacementPlan planCreativeSelectionPlacement(
    std::span<const CreativeObject> objects,
    const CreativeSelectionPlacementRequest& request) {
  CreativeSelectionPlacementPlan plan;
  plan.requested = true;
  plan.request = request;
  plan.objectCount = objects.size();
  if (objects.empty()) {
    plan.status = CreativeSelectionPlacementStatus::EmptySource;
    plan.reasonCode = "selection_placement_source_empty";
    return plan;
  }
  if (!validPlacementMode(request.mode) || request.quarterTurns > 3U ||
      !isFiniteCreativeVec3(request.sourceAnchor) ||
      !isFiniteCreativeVec3(request.targetAnchor)) {
    plan.status = CreativeSelectionPlacementStatus::InvalidRequest;
    plan.reasonCode = "selection_placement_request_invalid";
    return plan;
  }

  plan.objects.reserve(objects.size());
  for (const CreativeObject& source : objects) {
    if (!validPlacementObject(source)) {
      plan.failedObjectId = source.id;
      plan.status = CreativeSelectionPlacementStatus::InvalidSource;
      plan.reasonCode = "selection_placement_source_invalid";
      return plan;
    }
    CreativeObject transformed = transformPlacementObject(source, request);
    if (!validPlacementObject(transformed) ||
        !includePlacementObject(plan, transformed)) {
      plan.failedObjectId = source.id;
      plan.status = CreativeSelectionPlacementStatus::InvalidRequest;
      plan.reasonCode = "selection_placement_output_invalid";
      return plan;
    }
    plan.objects.push_back(std::move(transformed));
  }

  if (request.mode == CreativeSelectionPlacementMode::Move) {
    for (std::size_t index = 0; index < objects.size(); ++index) {
      const CreativeObject& source = objects[index];
      if (source.locked) {
        plan.failedObjectId = source.id;
        plan.status = CreativeSelectionPlacementStatus::LockedObject;
        plan.reasonCode = "selection_placement_object_locked";
        return plan;
      }
      const CreativeMutationKind unsupported =
          unsupportedPlacementMutation(source, plan.objects[index]);
      if (unsupported != CreativeMutationKind::Unknown) {
        plan.failedObjectId = source.id;
        plan.failedMutationKind = unsupported;
        plan.status = CreativeSelectionPlacementStatus::UnsupportedObject;
        plan.reasonCode = "selection_placement_object_unsupported";
        return plan;
      }
    }
  }

  plan.accepted = true;
  plan.status = CreativeSelectionPlacementStatus::Planned;
  plan.reasonCode = "selection_placement_planned";
  return plan;
}

CreativeSelectionPlacementReceipt placeDocumentObjectsAtomically(
    CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds,
    const CreativeSelectionPlacementRequest& request) {
  CreativeSelectionPlacementReceipt receipt;
  receipt.requested = true;
  receipt.requestedObjectCount = objectIds.size();
  receipt.revisionBefore = document.revision();
  receipt.revisionAfter = receipt.revisionBefore;
  if (request.mode != CreativeSelectionPlacementMode::Move) {
    receipt.status = CreativeSelectionPlacementStatus::InvalidRequest;
    receipt.reasonCode = "selection_placement_move_mode_required";
    return receipt;
  }
  if (objectIds.empty()) {
    receipt.status = CreativeSelectionPlacementStatus::EmptySource;
    receipt.reasonCode = "selection_placement_source_empty";
    return receipt;
  }

  const ResolvedObjects resolved =
      resolveObjectsInDocumentOrder(document, objectIds);
  if (resolved.objects.size() != objectIds.size()) {
    receipt.failedObjectId = resolved.missingObjectId;
    receipt.status = CreativeSelectionPlacementStatus::MissingObject;
    receipt.reasonCode = "selection_placement_object_missing";
    return receipt;
  }
  std::vector<CreativeObject> sourceObjects;
  sourceObjects.reserve(resolved.objects.size());
  for (const CreativeObject* object : resolved.objects) {
    sourceObjects.push_back(*object);
  }

  receipt.plan = planCreativeSelectionPlacement(sourceObjects, request);
  receipt.objectCount = receipt.plan.objectCount;
  receipt.failedObjectId = receipt.plan.failedObjectId;
  receipt.failedMutationKind = receipt.plan.failedMutationKind;
  if (!receipt.plan.accepted) {
    receipt.status = receipt.plan.status;
    receipt.reasonCode = receipt.plan.reasonCode;
    return receipt;
  }

  std::vector<CreativeMutationRequest> mutations;
  mutations.reserve(sourceObjects.size() * 3U);
  for (std::size_t index = 0; index < sourceObjects.size(); ++index) {
    appendPlacementMutations(sourceObjects[index], receipt.plan.objects[index],
                             mutations);
  }
  if (mutations.empty()) {
    receipt.accepted = true;
    receipt.status = CreativeSelectionPlacementStatus::NoChange;
    receipt.reasonCode = "selection_placement_no_change";
    return receipt;
  }

  receipt.mutationReceipt =
      applyDocumentMutationsAtomically(document, mutations);
  receipt.revisionAfter = document.revision();
  receipt.accepted = receipt.mutationReceipt.committed &&
                     documentMutationSucceeded(receipt.mutationReceipt.status);
  receipt.changed = receipt.accepted && receipt.mutationReceipt.changed;
  if (receipt.changed) {
    receipt.status = CreativeSelectionPlacementStatus::Applied;
    receipt.reasonCode = "selection_placement_applied";
    return receipt;
  }
  if (receipt.accepted) {
    receipt.status = CreativeSelectionPlacementStatus::NoChange;
    receipt.reasonCode = "selection_placement_no_change";
    return receipt;
  }

  receipt.status = CreativeSelectionPlacementStatus::Rejected;
  receipt.reasonCode = "selection_placement_rejected";
  for (const CreativeDocumentMutationReceipt& item :
       receipt.mutationReceipt.receipts) {
    if (documentMutationFailed(item.status)) {
      receipt.failedObjectId = item.objectId;
      receipt.failedMutationKind = item.mutationKind;
      break;
    }
  }
  return receipt;
}

}  // namespace iggy3d::creative
