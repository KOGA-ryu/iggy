#include "app/iggy3d/creative/tools/StructuralPlacement.hpp"

#include <algorithm>
#include <cmath>

#include "app/iggy3d/creative/Geometry.hpp"

namespace iggy3d::creative {
namespace {

constexpr double kAxisDifferenceEpsilon = 1.0e-9;

[[nodiscard]] bool validPlanForCore(
    const CreativeTransformedBounds& resolved) noexcept {
  return resolved.valid &&
         creativeVec3ToCoreChecked(resolved.center).converted &&
         creativeVec3ToCoreChecked(resolved.size).converted &&
         creativeVec3ToCoreChecked(resolved.worldBounds.min).converted &&
         creativeVec3ToCoreChecked(resolved.worldBounds.max).converted;
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
  return defaults.valid && isPositiveCreativeVec3(defaults.size) &&
         std::fabs(defaults.size.x - defaults.size.z) >
             kAxisDifferenceEpsilon;
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
  if (!isFiniteCreativeVec3(request.firstAnchor) ||
      !isFiniteCreativeVec3(request.secondAnchor)) {
    plan.status = CreativeStructuralSpanStatus::InvalidAnchor;
    return plan;
  }

  const double rawDx = request.secondAnchor.x - request.firstAnchor.x;
  const double rawDz = request.secondAnchor.z - request.firstAnchor.z;
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

  const CreativeBoundsMetrics defaults =
      measureCreativeBounds(descriptor.defaults.bounds);
  if (!defaults.valid || !isPositiveCreativeVec3(defaults.size)) {
    plan.status = CreativeStructuralSpanStatus::InvalidGeometry;
    return plan;
  }

  CreativeVec3 localSize = defaults.size;
  if (defaults.size.x > defaults.size.z + kAxisDifferenceEpsilon) {
    plan.spanAxis = CreativeStructuralSpanAxis::LocalX;
    localSize.x = length;
  } else if (defaults.size.z >
             defaults.size.x + kAxisDifferenceEpsilon) {
    plan.spanAxis = CreativeStructuralSpanAxis::LocalZ;
    localSize.z = length;
  } else {
    plan.status = CreativeStructuralSpanStatus::InvalidGeometry;
    return plan;
  }

  const double centerX = request.firstAnchor.x + rawDx * 0.5;
  const double centerZ = request.firstAnchor.z + rawDz * 0.5;
  const double centerY = request.firstAnchor.y + localSize.y * 0.5;
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
      plan.spanAxis == CreativeStructuralSpanAxis::LocalX
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
    case CreativeStructuralSpanStatus::ArithmeticOverflow:
      return "creative_structural_span_arithmetic_overflow";
  }
  return "creative_structural_span_invalid_status";
}

}  // namespace iggy3d::creative
