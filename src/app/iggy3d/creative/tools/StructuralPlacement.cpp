#include "app/iggy3d/creative/tools/StructuralPlacement.hpp"

#include <algorithm>
#include <cmath>

#include "app/iggy3d/creative/Geometry.hpp"

namespace iggy3d::creative {
namespace {

constexpr double kAxisDifferenceEpsilon = 1.0e-9;
constexpr double kPlanarRotationEpsilon = 1.0e-8;
constexpr double kEndpointEqualityEpsilon = 1.0e-9;

[[nodiscard]] bool validPlanForCore(
    const CreativeTransformedBounds& resolved) noexcept {
  return resolved.valid &&
         creativeVec3ToCoreChecked(resolved.center).converted &&
         creativeVec3ToCoreChecked(resolved.size).converted &&
         creativeVec3ToCoreChecked(resolved.worldBounds.min).converted &&
         creativeVec3ToCoreChecked(resolved.worldBounds.max).converted;
}

[[nodiscard]] bool spanAxisForDescriptor(
    const CreativeObjectDescriptor& descriptor,
    CreativeStructuralSpanAxis& axis) noexcept {
  const CreativeBoundsMetrics defaults =
      measureCreativeBounds(descriptor.defaults.bounds);
  if (!defaults.valid || !isPositiveCreativeVec3(defaults.size)) {
    return false;
  }
  if (defaults.size.x > defaults.size.z + kAxisDifferenceEpsilon) {
    axis = CreativeStructuralSpanAxis::LocalX;
    return true;
  }
  if (defaults.size.z > defaults.size.x + kAxisDifferenceEpsilon) {
    axis = CreativeStructuralSpanAxis::LocalZ;
    return true;
  }
  return false;
}

[[nodiscard]] bool endpointComesFirst(CreativeVec3 lhs,
                                      CreativeVec3 rhs) noexcept {
  return lhs.x < rhs.x - kEndpointEqualityEpsilon ||
         (std::fabs(lhs.x - rhs.x) <= kEndpointEqualityEpsilon &&
          lhs.z <= rhs.z);
}

void canonicalizeEndpoints(
    std::array<CreativeVec3, 2U>& endpoints) noexcept {
  if (!endpointComesFirst(endpoints[0], endpoints[1])) {
    std::swap(endpoints[0], endpoints[1]);
  }
}

[[nodiscard]] CreativeStructuralSpanPlan buildStructuralSpanPlan(
    CreativeObjectKind objectKind,
    CreativeStructuralSpanAxis spanAxis,
    CreativeVec3 localSize,
    CreativeVec3 scale,
    CreativeVec3 firstAnchor,
    CreativeVec3 secondAnchor) noexcept {
  CreativeStructuralSpanPlan plan;
  plan.objectKind = objectKind;
  plan.spanAxis = spanAxis;
  if (!isFiniteCreativeVec3(firstAnchor) ||
      !isFiniteCreativeVec3(secondAnchor)) {
    plan.status = CreativeStructuralSpanStatus::InvalidAnchor;
    return plan;
  }
  if (!isPositiveCreativeVec3(localSize) ||
      !isPositiveCreativeVec3(scale) ||
      spanAxis == CreativeStructuralSpanAxis::Count) {
    plan.status = CreativeStructuralSpanStatus::InvalidGeometry;
    return plan;
  }

  const double rawDx = secondAnchor.x - firstAnchor.x;
  const double rawDz = secondAnchor.z - firstAnchor.z;
  const double length = std::hypot(rawDx, rawDz);
  if (!std::isfinite(rawDx) || !std::isfinite(rawDz) ||
      !std::isfinite(length)) {
    plan.status = CreativeStructuralSpanStatus::ArithmeticOverflow;
    return plan;
  }
  if (length < kCreativeStructuralSpanMinimumMeters) {
    plan.status = CreativeStructuralSpanStatus::DegenerateSpan;
    return plan;
  }
  if (length > kCreativeStructuralSpanMaximumMeters) {
    plan.status = CreativeStructuralSpanStatus::SpanTooLong;
    return plan;
  }

  const double spanScale =
      spanAxis == CreativeStructuralSpanAxis::LocalX ? scale.x : scale.z;
  const double authoredLength = length / spanScale;
  if (!std::isfinite(authoredLength) || authoredLength <= 0.0) {
    plan.status = CreativeStructuralSpanStatus::ArithmeticOverflow;
    return plan;
  }
  if (spanAxis == CreativeStructuralSpanAxis::LocalX) {
    localSize.x = authoredLength;
  } else {
    localSize.z = authoredLength;
  }

  const double centerX = firstAnchor.x + rawDx * 0.5;
  const double centerZ = firstAnchor.z + rawDz * 0.5;
  const double centerY = firstAnchor.y + localSize.y * scale.y * 0.5;
  if (!std::isfinite(centerX) || !std::isfinite(centerY) ||
      !std::isfinite(centerZ)) {
    plan.status = CreativeStructuralSpanStatus::ArithmeticOverflow;
    return plan;
  }

  double canonicalDx = rawDx;
  double canonicalDz = rawDz;
  if (canonicalDx < -kAxisDifferenceEpsilon ||
      (std::fabs(canonicalDx) <= kAxisDifferenceEpsilon &&
       canonicalDz < 0.0)) {
    canonicalDx = -canonicalDx;
    canonicalDz = -canonicalDz;
  }
  const double yaw =
      spanAxis == CreativeStructuralSpanAxis::LocalX
          ? std::atan2(-canonicalDz, canonicalDx)
          : std::atan2(canonicalDx, canonicalDz);
  if (!std::isfinite(yaw)) {
    plan.status = CreativeStructuralSpanStatus::ArithmeticOverflow;
    return plan;
  }

  const CreativeVec3 center{centerX, centerY, centerZ};
  const CreativeVec3 half{localSize.x * 0.5, localSize.y * 0.5,
                          localSize.z * 0.5};
  plan.transform.position = center;
  plan.transform.rotationEulerRadians.y = yaw;
  plan.transform.scale = scale;
  plan.authoredBounds = {{center.x - half.x, center.y - half.y,
                          center.z - half.z},
                         {center.x + half.x, center.y + half.y,
                          center.z + half.z}};
  const CreativeTransformedBounds resolved =
      resolveCreativeTransformedBounds(plan.authoredBounds, plan.transform);
  if (!validPlanForCore(resolved)) {
    plan.status = CreativeStructuralSpanStatus::ArithmeticOverflow;
    return plan;
  }

  plan.spanLengthMeters = length;
  plan.status = CreativeStructuralSpanStatus::Ready;
  plan.accepted = true;
  return plan;
}

}  // namespace

bool descriptorSupportsCreativeStructuralSpan(
    const CreativeObjectDescriptor& descriptor) noexcept {
  if (!descriptor.placementPolicy.enabled || !descriptor.hasTransform ||
      !descriptor.hasBounds ||
      descriptor.placementPolicy.gesturePolicy !=
          CreativePlacementGesturePolicy::HorizontalSpan ||
      descriptor.placementPolicy.targetPolicy !=
          CreativePlacementTargetPolicy::AdjacentCell ||
      descriptor.placementPolicy.occupancyPolicy !=
          CreativePlacementOccupancyPolicy::AllowOverlap ||
      descriptor.placementPolicy.storagePolicy !=
          CreativePlacementStoragePolicy::AuthoredObject) {
    return false;
  }
  const CreativeBoundsMetrics defaults =
      measureCreativeBounds(descriptor.defaults.bounds);
  CreativeStructuralSpanAxis axis = CreativeStructuralSpanAxis::Count;
  return defaults.valid && isPositiveCreativeVec3(defaults.size) &&
         spanAxisForDescriptor(descriptor, axis);
}

CreativeStructuralSpanPlan planCreativeStructuralSpan(
    const CreativeStructuralSpanRequest& request) noexcept {
  CreativeStructuralSpanPlan plan;
  plan.objectKind = request.objectKind;
  const CreativeObjectDescriptor& descriptor =
      describeObject(request.objectKind);
  if (!descriptorSupportsCreativeStructuralSpan(descriptor)) {
    return plan;
  }
  const CreativeBoundsMetrics defaults =
      measureCreativeBounds(descriptor.defaults.bounds);
  CreativeStructuralSpanAxis spanAxis = CreativeStructuralSpanAxis::Count;
  if (!defaults.valid || !isPositiveCreativeVec3(defaults.size) ||
      !spanAxisForDescriptor(descriptor, spanAxis)) {
    plan.status = CreativeStructuralSpanStatus::InvalidGeometry;
    return plan;
  }
  return buildStructuralSpanPlan(request.objectKind, spanAxis, defaults.size,
                                 {1.0, 1.0, 1.0}, request.firstAnchor,
                                 request.secondAnchor);
}

CreativeStructuralSpanInstance resolveCreativeStructuralSpan(
    CreativeObjectKind objectKind,
    CreativeTransform transform,
    CreativeBounds authoredBounds) noexcept {
  CreativeStructuralSpanInstance instance;
  instance.objectKind = objectKind;
  const CreativeObjectDescriptor& descriptor = describeObject(objectKind);
  if (!descriptorSupportsCreativeStructuralSpan(descriptor)) {
    return instance;
  }
  if (!isFiniteCreativeVec3(transform.position) ||
      !isFiniteCreativeVec3(transform.rotationEulerRadians) ||
      !isPositiveCreativeVec3(transform.scale)) {
    instance.status = CreativeStructuralSpanStatus::InvalidGeometry;
    return instance;
  }
  if (std::fabs(transform.rotationEulerRadians.x) >
          kPlanarRotationEpsilon ||
      std::fabs(transform.rotationEulerRadians.z) >
          kPlanarRotationEpsilon) {
    instance.status = CreativeStructuralSpanStatus::UnsupportedTransform;
    return instance;
  }

  CreativeStructuralSpanAxis spanAxis = CreativeStructuralSpanAxis::Count;
  const CreativeBoundsMetrics metrics = measureCreativeBounds(authoredBounds);
  if (!metrics.valid || !isPositiveCreativeVec3(metrics.size) ||
      !spanAxisForDescriptor(descriptor, spanAxis)) {
    instance.status = CreativeStructuralSpanStatus::InvalidGeometry;
    return instance;
  }
  const CreativeTransformedBounds resolved =
      resolveCreativeTransformedBounds(authoredBounds, transform);
  if (!resolved.valid || !validPlanForCore(resolved)) {
    instance.status = CreativeStructuralSpanStatus::ArithmeticOverflow;
    return instance;
  }

  const double length = spanAxis == CreativeStructuralSpanAxis::LocalX
                            ? metrics.size.x * transform.scale.x
                            : metrics.size.z * transform.scale.z;
  if (!std::isfinite(length)) {
    instance.status = CreativeStructuralSpanStatus::ArithmeticOverflow;
    return instance;
  }
  if (length < kCreativeStructuralSpanMinimumMeters) {
    instance.status = CreativeStructuralSpanStatus::DegenerateSpan;
    return instance;
  }
  if (length > kCreativeStructuralSpanMaximumMeters) {
    instance.status = CreativeStructuralSpanStatus::SpanTooLong;
    return instance;
  }

  const double yaw = transform.rotationEulerRadians.y;
  const CreativeVec3 direction =
      spanAxis == CreativeStructuralSpanAxis::LocalX
          ? CreativeVec3{std::cos(yaw), 0.0, -std::sin(yaw)}
          : CreativeVec3{std::sin(yaw), 0.0, std::cos(yaw)};
  const double bottomY = resolved.center.y - resolved.size.y * 0.5;
  const CreativeVec3 halfSpan{direction.x * length * 0.5, 0.0,
                              direction.z * length * 0.5};
  instance.endpoints = {
      CreativeVec3{resolved.center.x - halfSpan.x, bottomY,
                   resolved.center.z - halfSpan.z},
      CreativeVec3{resolved.center.x + halfSpan.x, bottomY,
                   resolved.center.z + halfSpan.z}};
  canonicalizeEndpoints(instance.endpoints);
  if (!isFiniteCreativeVec3(instance.endpoints[0]) ||
      !isFiniteCreativeVec3(instance.endpoints[1]) ||
      !creativeVec3ToCoreChecked(instance.endpoints[0]).converted ||
      !creativeVec3ToCoreChecked(instance.endpoints[1]).converted) {
    instance.status = CreativeStructuralSpanStatus::ArithmeticOverflow;
    instance.endpoints = {};
    return instance;
  }

  instance.spanAxis = spanAxis;
  instance.spanLengthMeters = length;
  instance.status = CreativeStructuralSpanStatus::Ready;
  instance.accepted = true;
  return instance;
}

CreativeStructuralSpanEditPlan planCreativeStructuralSpanEdit(
    const CreativeStructuralSpanEditRequest& request) noexcept {
  CreativeStructuralSpanEditPlan edit;
  edit.objectKind = request.objectKind;
  edit.endpoint = request.endpoint;
  const CreativeStructuralSpanInstance source = resolveCreativeStructuralSpan(
      request.objectKind, request.sourceTransform, request.sourceBounds);
  edit.status = source.status;
  edit.spanAxis = source.spanAxis;
  edit.sourceEndpoints = source.endpoints;
  edit.resultEndpoints = source.endpoints;
  if (!source.accepted) {
    return edit;
  }
  if (request.endpoint != CreativeStructuralSpanEndpoint::First &&
      request.endpoint != CreativeStructuralSpanEndpoint::Second) {
    edit.status = CreativeStructuralSpanStatus::InvalidEndpoint;
    return edit;
  }
  if (!isFiniteCreativeVec3(request.targetAnchor)) {
    edit.status = CreativeStructuralSpanStatus::InvalidAnchor;
    return edit;
  }

  const std::size_t endpointIndex =
      static_cast<std::size_t>(request.endpoint);
  CreativeVec3 target = request.targetAnchor;
  target.y = source.endpoints[0].y;
  const CreativeVec3& original = source.endpoints[endpointIndex];
  if (std::hypot(target.x - original.x, target.z - original.z) <=
      kEndpointEqualityEpsilon) {
    edit.transform = request.sourceTransform;
    edit.authoredBounds = request.sourceBounds;
    edit.spanLengthMeters = source.spanLengthMeters;
    edit.status = CreativeStructuralSpanStatus::Ready;
    edit.accepted = true;
    return edit;
  }

  std::array<CreativeVec3, 2U> requestedEndpoints = source.endpoints;
  requestedEndpoints[endpointIndex] = target;
  const CreativeBoundsMetrics sourceMetrics =
      measureCreativeBounds(request.sourceBounds);
  const CreativeStructuralSpanPlan replanned = buildStructuralSpanPlan(
      request.objectKind, source.spanAxis, sourceMetrics.size,
      request.sourceTransform.scale, requestedEndpoints[0],
      requestedEndpoints[1]);
  edit.status = replanned.status;
  if (!replanned.accepted) {
    return edit;
  }

  const CreativeStructuralSpanInstance result = resolveCreativeStructuralSpan(
      request.objectKind, replanned.transform, replanned.authoredBounds);
  if (!result.accepted) {
    edit.status = result.status;
    return edit;
  }
  edit.resultEndpoints = result.endpoints;
  edit.transform = replanned.transform;
  edit.authoredBounds = replanned.authoredBounds;
  edit.spanLengthMeters = replanned.spanLengthMeters;
  edit.status = CreativeStructuralSpanStatus::Ready;
  edit.accepted = true;
  edit.changed = true;
  return edit;
}

std::string_view toString(CreativeStructuralSpanStatus status) noexcept {
  switch (status) {
    case CreativeStructuralSpanStatus::Ready:
      return "creative_structural_span_ready";
    case CreativeStructuralSpanStatus::UnsupportedDescriptor:
      return "creative_structural_span_unsupported_descriptor";
    case CreativeStructuralSpanStatus::InvalidAnchor:
      return "creative_structural_span_invalid_anchor";
    case CreativeStructuralSpanStatus::DegenerateSpan:
      return "creative_structural_span_degenerate";
    case CreativeStructuralSpanStatus::SpanTooLong:
      return "creative_structural_span_too_long";
    case CreativeStructuralSpanStatus::InvalidGeometry:
      return "creative_structural_span_invalid_geometry";
    case CreativeStructuralSpanStatus::UnsupportedTransform:
      return "creative_structural_span_unsupported_transform";
    case CreativeStructuralSpanStatus::InvalidEndpoint:
      return "creative_structural_span_invalid_endpoint";
    case CreativeStructuralSpanStatus::ArithmeticOverflow:
      return "creative_structural_span_arithmetic_overflow";
  }
  return "creative_structural_span_invalid_status";
}

}  // namespace iggy3d::creative
