#include "app/iggy3d/creative/tools/SelectionPlacement.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/Hierarchy.hpp"
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

[[nodiscard]] CreativeVec3 multiply(CreativeVec3 lhs,
                                    CreativeVec3 rhs) noexcept {
  return {lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z};
}

[[nodiscard]] bool samePathPoints(
    std::span<const CreativePathPoint> lhs,
    std::span<const CreativePathPoint> rhs) noexcept {
  return lhs.size() == rhs.size() &&
         std::equal(lhs.begin(), lhs.end(), rhs.begin(),
                    [](const CreativePathPoint& left,
                       const CreativePathPoint& right) {
                      return creativeVec3ExactlyEqual(left.position,
                                                      right.position) &&
                             left.dwellSeconds == right.dwellSeconds &&
                             left.outgoingSpeedMultiplier ==
                                 right.outgoingSpeedMultiplier;
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

[[nodiscard]] bool validPivotMode(
    CreativeSelectionPlacementPivotMode mode) noexcept {
  return static_cast<std::size_t>(mode) <
         static_cast<std::size_t>(CreativeSelectionPlacementPivotMode::Count);
}

[[nodiscard]] bool validCoordinateSpace(
    CreativeSelectionPlacementCoordinateSpace space) noexcept {
  return static_cast<std::size_t>(space) <
         static_cast<std::size_t>(
             CreativeSelectionPlacementCoordinateSpace::Count);
}

[[nodiscard]] bool placementTranslationRequested(
    const CreativeSelectionPlacementRequest& request) noexcept {
  return !creativeVec3ExactlyEqual(request.sourceAnchor, request.targetAnchor);
}

[[nodiscard]] bool placementRotationRequested(
    const CreativeSelectionPlacementRequest& request) noexcept {
  return request.quarterTurns != 0U ||
         (request.hasAxisAngleRotation &&
          std::abs(request.rotationRadians) > 1.0e-12);
}

[[nodiscard]] bool placementScaleRequested(
    const CreativeSelectionPlacementRequest& request) noexcept {
  return !creativeVec3ExactlyEqual(request.scaleFactor, {1.0, 1.0, 1.0});
}

[[nodiscard]] bool uniformScale(CreativeVec3 factor) noexcept {
  return std::abs(factor.x - factor.y) <= 1.0e-12 &&
         std::abs(factor.x - factor.z) <= 1.0e-12;
}

struct PlacementBasis {
  CreativeVec3 x{1.0, 0.0, 0.0};
  CreativeVec3 y{0.0, 1.0, 0.0};
  CreativeVec3 z{0.0, 0.0, 1.0};
};

[[nodiscard]] bool cardinalDirection(CreativeVec3 direction) noexcept {
  const double ax = std::abs(direction.x);
  const double ay = std::abs(direction.y);
  const double az = std::abs(direction.z);
  const double maximum = std::max({ax, ay, az});
  const double remainder = ax + ay + az - maximum;
  return std::abs(maximum - 1.0) <= 1.0e-9 && remainder <= 1.0e-9;
}

[[nodiscard]] bool cardinalBasis(const PlacementBasis& basis) noexcept {
  return cardinalDirection(basis.x) && cardinalDirection(basis.y) &&
         cardinalDirection(basis.z);
}

[[nodiscard]] bool quarterTurnRadians(double radians) noexcept {
  const double quarterTurns = radians / (std::numbers::pi * 0.5);
  return std::isfinite(quarterTurns) &&
         std::abs(quarterTurns - std::round(quarterTurns)) <= 1.0e-9;
}

[[nodiscard]] PlacementBasis placementBasis(
    CreativeSelectionPlacementCoordinateSpace space,
    CreativeVec3 eulerRadians) noexcept {
  if (space == CreativeSelectionPlacementCoordinateSpace::World) {
    return {};
  }
  return {
      rotateCreativeVectorEulerXyz({1.0, 0.0, 0.0}, eulerRadians),
      rotateCreativeVectorEulerXyz({0.0, 1.0, 0.0}, eulerRadians),
      rotateCreativeVectorEulerXyz({0.0, 0.0, 1.0}, eulerRadians),
  };
}

[[nodiscard]] double dot(CreativeVec3 lhs, CreativeVec3 rhs) noexcept {
  return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

[[nodiscard]] CreativeVec3 scale(CreativeVec3 value,
                                 double factor) noexcept {
  return {value.x * factor, value.y * factor, value.z * factor};
}

[[nodiscard]] CreativeVec3 basisAxis(
    const PlacementBasis& basis,
    CreativeSelectionPlacementAxis axis) noexcept {
  switch (axis) {
    case CreativeSelectionPlacementAxis::X: return basis.x;
    case CreativeSelectionPlacementAxis::Y: return basis.y;
    case CreativeSelectionPlacementAxis::Z: return basis.z;
    case CreativeSelectionPlacementAxis::Free:
    case CreativeSelectionPlacementAxis::Count:
      return {};
  }
  return {};
}

[[nodiscard]] CreativeVec3 basisAxis(
    const PlacementBasis& basis,
    CreativeAxis3 axis) noexcept {
  switch (axis) {
    case CreativeAxis3::X: return basis.x;
    case CreativeAxis3::Y: return basis.y;
    case CreativeAxis3::Z: return basis.z;
    case CreativeAxis3::Count: return {};
  }
  return {};
}

[[nodiscard]] CreativeVec3 toBasis(CreativeVec3 value,
                                   const PlacementBasis& basis) noexcept {
  return {dot(value, basis.x), dot(value, basis.y), dot(value, basis.z)};
}

[[nodiscard]] CreativeVec3 fromBasis(CreativeVec3 value,
                                     const PlacementBasis& basis) noexcept {
  return add(add(scale(basis.x, value.x), scale(basis.y, value.y)),
             scale(basis.z, value.z));
}

[[nodiscard]] CreativeVec3 constrainedDisplacement(
    CreativeVec3 displacement,
    CreativeSelectionPlacementAxis axis,
    CreativeSelectionPlacementCoordinateSpace coordinateSpace,
    CreativeVec3 coordinateBasisEulerRadians,
    double snapStepMeters) noexcept {
  if (coordinateSpace == CreativeSelectionPlacementCoordinateSpace::Local &&
      axis != CreativeSelectionPlacementAxis::Free &&
      axis != CreativeSelectionPlacementAxis::Count) {
    const PlacementBasis basis =
        placementBasis(coordinateSpace, coordinateBasisEulerRadians);
    const CreativeVec3 direction = basisAxis(basis, axis);
    const double displacementOnAxis = dot(displacement, direction);
    return scale(direction, iggy3d::snapScalarToGrid(
                                displacementOnAxis, snapStepMeters, 0.0));
  }
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
                  CreativeSelectionPlacementAxis axis,
                  CreativeSelectionPlacementCoordinateSpace coordinateSpace,
                  CreativeVec3 coordinateBasisEulerRadians) noexcept {
  if (coordinateSpace == CreativeSelectionPlacementCoordinateSpace::Local &&
      axis != CreativeSelectionPlacementAxis::Free &&
      axis != CreativeSelectionPlacementAxis::Count) {
    const PlacementBasis basis =
        placementBasis(coordinateSpace, coordinateBasisEulerRadians);
    const CreativeVec3 direction = basisAxis(basis, axis);
    displacement = add(displacement,
                       scale(direction, dot(nudgeOffset, direction)));
    return;
  }
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
                  CreativeSelectionPlacementCoordinateSpace coordinateSpace,
                  CreativeVec3 coordinateBasisEulerRadians,
                  double delta) noexcept {
  if (coordinateSpace == CreativeSelectionPlacementCoordinateSpace::Local &&
      axis != CreativeSelectionPlacementAxis::Free &&
      axis != CreativeSelectionPlacementAxis::Count) {
    const PlacementBasis basis =
        placementBasis(coordinateSpace, coordinateBasisEulerRadians);
    offset = add(offset, scale(basisAxis(basis, axis), delta));
    return;
  }
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
  const PlacementBasis basis = placementBasis(
      request.coordinateSpace, request.coordinateBasisEulerRadians);
  if (request.coordinateSpace ==
      CreativeSelectionPlacementCoordinateSpace::Local) {
    offset = toBasis(offset, basis);
  }
  offset = multiply(offset, request.scaleFactor);
  if (request.mirrorX) {
    offset.x = -offset.x;
  }
  if (request.mirrorZ) {
    offset.z = -offset.z;
  }
  if (request.coordinateSpace ==
      CreativeSelectionPlacementCoordinateSpace::Local) {
    offset = fromBasis(offset, basis);
  }
  if (request.hasAxisAngleRotation) {
    if (request.coordinateSpace ==
        CreativeSelectionPlacementCoordinateSpace::Local) {
      return rotateCreativeVectorAroundAxis(
          offset, basisAxis(basis, request.rotationAxis),
          request.rotationRadians);
    }
    return rotateCreativeVectorAxisAngle(offset, request.rotationAxis,
                                         request.rotationRadians);
  }
  if (request.coordinateSpace ==
          CreativeSelectionPlacementCoordinateSpace::Local &&
      request.quarterTurns != 0U) {
    return rotateCreativeVectorAroundAxis(
        offset, basis.y,
        static_cast<double>(request.quarterTurns) * std::numbers::pi * 0.5);
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

[[nodiscard]] CreativeVec3 transformPlacementPointAroundObjectOrigin(
    CreativeVec3 point,
    CreativeVec3 objectOrigin,
    const CreativeSelectionPlacementRequest& request) noexcept {
  const CreativeVec3 translation =
      subtract(request.targetAnchor, request.sourceAnchor);
  return add(add(objectOrigin, translation),
             transformPlacementOffset(subtract(point, objectOrigin), request));
}

[[nodiscard]] CreativeMutationKind placementMutationForOperation(
    const CreativeObject& object,
    bool translation,
    bool rotation,
    bool scaleRequested,
    bool mirrorRequested) noexcept {
  if (objectHasTransform(object.kind)) {
    if (translation) return CreativeMutationKind::Move;
    if (rotation || mirrorRequested) return CreativeMutationKind::Rotate;
    if (scaleRequested) return CreativeMutationKind::Scale;
  }
  if (objectHasBounds(object.kind)) {
    return CreativeMutationKind::SetBounds;
  }
  if (objectStoresPathPoints(object.kind)) {
    return CreativeMutationKind::SetPatrolRoute;
  }
  return CreativeMutationKind::Unknown;
}

[[nodiscard]] bool placementRequestSupportedByObject(
    const CreativeObject& object,
    const CreativeSelectionPlacementRequest& request,
    CreativeMutationKind& failedMutation,
    std::string_view& reasonCode) noexcept {
  const CreativeObjectTransformCapabilities capabilities =
      creativeObjectTransformCapabilities(object.kind);
  const bool translation = placementTranslationRequested(request);
  const bool rotation = placementRotationRequested(request);
  const bool scaleRequested = placementScaleRequested(request);
  const bool mirrorRequested = request.mirrorX || request.mirrorZ;
  failedMutation = placementMutationForOperation(
      object, translation, rotation, scaleRequested, mirrorRequested);

  if (translation && !capabilities.translate) {
    reasonCode = "selection_placement_translation_unsupported";
    return false;
  }
  if (rotation &&
      capabilities.rotation == CreativeObjectRotationSupport::None) {
    reasonCode = "selection_placement_rotation_unsupported";
    return false;
  }
  if (scaleRequested &&
      capabilities.scale == CreativeObjectScaleSupport::None) {
    reasonCode = "selection_placement_scale_unsupported";
    return false;
  }
  if (scaleRequested &&
      capabilities.scale == CreativeObjectScaleSupport::Uniform &&
      !uniformScale(request.scaleFactor)) {
    reasonCode = "selection_placement_nonuniform_scale_unsupported";
    return false;
  }
  if (mirrorRequested && !capabilities.mirror) {
    reasonCode = "selection_placement_mirror_unsupported";
    return false;
  }

  const bool boundsOnly =
      !objectHasTransform(object.kind) && objectHasBounds(object.kind);
  if (!boundsOnly) {
    return true;
  }

  const PlacementBasis basis = placementBasis(
      request.coordinateSpace, request.coordinateBasisEulerRadians);
  const bool basisRepresentable =
      request.coordinateSpace == CreativeSelectionPlacementCoordinateSpace::World ||
      cardinalBasis(basis);
  if (rotation &&
      (capabilities.rotation != CreativeObjectRotationSupport::QuarterTurns ||
       (request.hasAxisAngleRotation &&
        !quarterTurnRadians(request.rotationRadians)) ||
       !basisRepresentable)) {
    reasonCode = "selection_placement_rotation_not_representable";
    return false;
  }
  if ((mirrorRequested || (scaleRequested && !uniformScale(request.scaleFactor))) &&
      !basisRepresentable) {
    reasonCode = "selection_placement_local_bounds_not_representable";
    return false;
  }
  return true;
}

[[nodiscard]] CreativeVec3 transformPlacementRotation(
    CreativeVec3 eulerRadians,
    const CreativeSelectionPlacementRequest& request) noexcept {
  const PlacementBasis basis = placementBasis(
      request.coordinateSpace, request.coordinateBasisEulerRadians);
  double transformed = eulerRadians.y;
  if (request.mirrorX) {
    transformed = -transformed;
  }
  if (request.mirrorZ) {
    transformed = std::numbers::pi - transformed;
  }
  if (request.coordinateSpace ==
      CreativeSelectionPlacementCoordinateSpace::World) {
    transformed += static_cast<double>(request.quarterTurns) *
                   std::numbers::pi * 0.5;
  }
  eulerRadians.y = std::remainder(transformed, std::numbers::pi * 2.0);
  if (request.hasAxisAngleRotation) {
    if (request.coordinateSpace ==
        CreativeSelectionPlacementCoordinateSpace::Local) {
      return composeCreativeWorldAxisRotation(
          eulerRadians, basisAxis(basis, request.rotationAxis),
          request.rotationRadians);
    }
    return composeCreativeWorldAxisRotation(
        eulerRadians, request.rotationAxis, request.rotationRadians);
  }
  if (request.coordinateSpace ==
          CreativeSelectionPlacementCoordinateSpace::Local &&
      request.quarterTurns != 0U) {
    return composeCreativeWorldAxisRotation(
        eulerRadians, basis.y,
        static_cast<double>(request.quarterTurns) * std::numbers::pi * 0.5);
  }
  return eulerRadians;
}

[[nodiscard]] CreativeBounds transformPlacementBounds(
    CreativeBounds bounds,
    const CreativeSelectionPlacementRequest& request,
    CreativeVec3 objectOrigin) noexcept {
  CreativeBounds output{};
  bool initialized = false;
  for (std::size_t index = 0; index < 8U; ++index) {
    const CreativeVec3 corner{
        (index & 1U) != 0U ? bounds.max.x : bounds.min.x,
        (index & 2U) != 0U ? bounds.max.y : bounds.min.y,
        (index & 4U) != 0U ? bounds.max.z : bounds.min.z,
    };
    const CreativeVec3 transformed =
        request.pivotMode ==
                CreativeSelectionPlacementPivotMode::IndividualOrigins
            ? transformPlacementPointAroundObjectOrigin(corner, objectOrigin,
                                                        request)
            : transformPlacementPoint(corner, request);
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
                       return isValidCreativePathPoint(point);
                     });
}

[[nodiscard]] CreativeObject transformPlacementObject(
    const CreativeObject& source,
    const CreativeSelectionPlacementRequest& request) {
  CreativeObject output = source;
  CreativeVec3 objectOrigin{};
  if (!resolveCreativeSelectionPlacementObjectOrigin(source, objectOrigin)) {
    return {};
  }
  const auto transformPoint = [&](CreativeVec3 point) {
    return request.pivotMode ==
                   CreativeSelectionPlacementPivotMode::IndividualOrigins
               ? transformPlacementPointAroundObjectOrigin(point, objectOrigin,
                                                           request)
               : transformPlacementPoint(point, request);
  };
  if (objectHasTransform(source.kind)) {
    output.transform.position = transformPoint(source.transform.position);
    output.transform.rotationEulerRadians = transformPlacementRotation(
        source.transform.rotationEulerRadians, request);
    output.transform.scale =
        multiply(source.transform.scale, request.scaleFactor);
    if (objectHasBounds(source.kind)) {
      output.bounds = translateBounds(
          source.bounds,
          subtract(output.transform.position, source.transform.position));
    }
  } else if (objectHasBounds(source.kind)) {
    output.bounds = transformPlacementBounds(source.bounds, request, objectOrigin);
  }
  for (CreativePathPoint& point : output.pathPoints) {
    point.position = transformPoint(point.position);
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
  if (objectHasTransform(source.kind) &&
      !creativeVec3ExactlyEqual(source.transform.scale,
                                transformed.transform.scale) &&
      !descriptorAllowsMutation(source.kind, CreativeMutationKind::Scale)) {
    return CreativeMutationKind::Scale;
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
  if (objectHasTransform(source.kind) &&
      !creativeVec3ExactlyEqual(source.transform.scale,
                                transformed.transform.scale)) {
    mutations.push_back(
        {0, source.id, CreativeMutationKind::Scale,
         CreativeMutationPayload{ScaleMutation{transformed.transform.scale}}});
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

std::string_view toString(
    CreativeSelectionPlacementPivotMode mode) noexcept {
  switch (mode) {
    case CreativeSelectionPlacementPivotMode::SharedAnchor:
      return "SharedAnchor";
    case CreativeSelectionPlacementPivotMode::IndividualOrigins:
      return "IndividualOrigins";
    case CreativeSelectionPlacementPivotMode::Count:
      break;
  }
  return "INVALID";
}

std::string_view toString(
    CreativeSelectionPlacementCoordinateSpace space) noexcept {
  switch (space) {
    case CreativeSelectionPlacementCoordinateSpace::World: return "WORLD";
    case CreativeSelectionPlacementCoordinateSpace::Local: return "LOCAL";
    case CreativeSelectionPlacementCoordinateSpace::Count: break;
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
      !validCoordinateSpace(request.coordinateSpace) ||
      !isFiniteCreativeVec3(request.sourceAnchor) ||
      !isFiniteCreativeVec3(request.aimedAnchor) ||
      !isFiniteCreativeVec3(request.nudgeOffset) ||
      !isFiniteCreativeVec3(request.coordinateBasisEulerRadians) ||
      !std::isfinite(request.snapStepMeters) ||
      request.snapStepMeters <= 0.0) {
    result.status = CreativeSelectionPlacementTargetStatus::InvalidRequest;
    result.reasonCode = "selection_placement_target_invalid";
    return result;
  }

  result.displacement = constrainedDisplacement(
      subtract(request.aimedAnchor, request.sourceAnchor), request.axis,
      request.coordinateSpace, request.coordinateBasisEulerRadians,
      request.snapStepMeters);
  addAxisNudge(result.displacement, request.nudgeOffset, request.axis,
               request.coordinateSpace,
               request.coordinateBasisEulerRadians);
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
      !validCoordinateSpace(request.coordinateSpace) ||
      !isFiniteCreativeVec3(request.offset) ||
      !isFiniteCreativeVec3(request.coordinateBasisEulerRadians) ||
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
  addNudgeStep(receipt.offset, request.axis, request.coordinateSpace,
               request.coordinateBasisEulerRadians, delta);
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

bool resolveCreativeSelectionPlacementObjectOrigin(
    const CreativeObject& object,
    CreativeVec3& origin) noexcept {
  origin = {};
  if (objectHasTransform(object.kind)) {
    if (!isFiniteCreativeVec3(object.transform.position)) {
      return false;
    }
    origin = object.transform.position;
    return true;
  }

  const CreativeObjectWorldExtent extent =
      resolveCreativeObjectWorldExtent(object);
  if (!extent.valid || !isFiniteCreativeVec3(extent.min) ||
      !isFiniteCreativeVec3(extent.max)) {
    return false;
  }
  origin = {
      extent.min.x + (extent.max.x - extent.min.x) * 0.5,
      extent.min.y + (extent.max.y - extent.min.y) * 0.5,
      extent.min.z + (extent.max.z - extent.min.z) * 0.5,
  };
  return isFiniteCreativeVec3(origin);
}

CreativeSelectionPlacementCapabilities
resolveCreativeSelectionPlacementCapabilities(
    std::span<const CreativeObject> objects) noexcept {
  CreativeSelectionPlacementCapabilities result;
  result.objectCount = objects.size();
  if (objects.empty()) {
    return result;
  }

  result.resolved = true;
  result.translate = true;
  result.rotation = CreativeObjectRotationSupport::Arbitrary;
  result.scale = CreativeObjectScaleSupport::NonUniform;
  result.mirror = true;
  for (const CreativeObject& object : objects) {
    if (object.kind == CreativeObjectKind::Unknown ||
        object.kind == CreativeObjectKind::Count) {
      return {};
    }
    const CreativeObjectTransformCapabilities objectCapabilities =
        creativeObjectTransformCapabilities(object.kind);
    result.translate = result.translate && objectCapabilities.translate;
    result.rotation = static_cast<CreativeObjectRotationSupport>(std::min(
        static_cast<std::uint8_t>(result.rotation),
        static_cast<std::uint8_t>(objectCapabilities.rotation)));
    result.scale = static_cast<CreativeObjectScaleSupport>(std::min(
        static_cast<std::uint8_t>(result.scale),
        static_cast<std::uint8_t>(objectCapabilities.scale)));
    result.mirror = result.mirror && objectCapabilities.mirror;
  }
  return result;
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
  if (!validPlacementMode(request.mode) ||
      !validPivotMode(request.pivotMode) || request.quarterTurns > 3U ||
      !validCoordinateSpace(request.coordinateSpace) ||
      !isFiniteCreativeVec3(request.sourceAnchor) ||
      !isFiniteCreativeVec3(request.targetAnchor) ||
      !isFiniteCreativeVec3(request.coordinateBasisEulerRadians) ||
      !isPositiveCreativeVec3(request.scaleFactor) ||
      !isValidCreativeAxis3(request.rotationAxis) ||
      !std::isfinite(request.rotationRadians) ||
      (request.hasAxisAngleRotation && request.quarterTurns != 0U)) {
    plan.status = CreativeSelectionPlacementStatus::InvalidRequest;
    plan.reasonCode = "selection_placement_request_invalid";
    return plan;
  }

  const bool transformRequested =
      placementTranslationRequested(request) ||
      placementRotationRequested(request) ||
      placementScaleRequested(request) || request.mirrorX || request.mirrorZ;
  std::unordered_set<CreativeObjectId> sourceIds;
  sourceIds.reserve(objects.size());
  for (const CreativeObject& object : objects) {
    sourceIds.insert(object.id);
  }

  plan.objects.reserve(objects.size());
  for (const CreativeObject& source : objects) {
    if (!validPlacementObject(source)) {
      plan.failedObjectId = source.id;
      plan.status = CreativeSelectionPlacementStatus::InvalidSource;
      plan.reasonCode = "selection_placement_source_invalid";
      return plan;
    }
    CreativeMutationKind failedMutation = CreativeMutationKind::Unknown;
    std::string_view unsupportedReason;
    if (!placementRequestSupportedByObject(
            source, request, failedMutation, unsupportedReason)) {
      plan.failedObjectId = source.id;
      plan.failedMutationKind = failedMutation;
      plan.status = CreativeSelectionPlacementStatus::UnsupportedObject;
      plan.reasonCode = unsupportedReason;
      return plan;
    }
    if (request.mode == CreativeSelectionPlacementMode::Move &&
        transformRequested && source.parentId.has_value() &&
        !sourceIds.contains(*source.parentId)) {
      plan.failedObjectId = source.id;
      plan.failedMutationKind = CreativeMutationKind::Move;
      plan.status = CreativeSelectionPlacementStatus::UnsupportedObject;
      plan.reasonCode = "selection_placement_external_parent";
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
  for (const CreativeObject* object : resolved.objects) {
    if (creativeObjectEffectivelyLocked(document, object->id)) {
      receipt.failedObjectId = object->id;
      receipt.failedMutationKind = CreativeMutationKind::Move;
      receipt.status = CreativeSelectionPlacementStatus::LockedObject;
      receipt.reasonCode = "selection_placement_object_locked";
      return receipt;
    }
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
