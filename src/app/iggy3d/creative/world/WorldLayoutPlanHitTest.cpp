#include "app/iggy3d/creative/world/WorldLayoutPlanHitTest.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace iggy3d::creative {
namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 2.0 * kPi;
constexpr double kGeometryEpsilon = 1.0e-9;

bool finite(CreativeWorldLayoutPlanPoint point) noexcept {
  return std::isfinite(point.x) && std::isfinite(point.z);
}

double squaredDistance(CreativeWorldLayoutPlanPoint lhs,
                       CreativeWorldLayoutPlanPoint rhs) noexcept {
  const double dx = lhs.x - rhs.x;
  const double dz = lhs.z - rhs.z;
  return dx * dx + dz * dz;
}

double pointSegmentDistance(CreativeWorldLayoutPlanPoint point,
                            CreativeWorldLayoutPlanPoint start,
                            CreativeWorldLayoutPlanPoint end) noexcept {
  const double dx = end.x - start.x;
  const double dz = end.z - start.z;
  const double lengthSquared = dx * dx + dz * dz;
  if (lengthSquared <= kGeometryEpsilon) {
    return std::sqrt(squaredDistance(point, start));
  }
  const double t = std::clamp(
      ((point.x - start.x) * dx + (point.z - start.z) * dz) /
          lengthSquared,
      0.0, 1.0);
  const CreativeWorldLayoutPlanPoint closest{start.x + dx * t,
                                              start.z + dz * t};
  return std::sqrt(squaredDistance(point, closest));
}

bool pointInPolygon(const CreativeWorldLayoutPlanPrimitive& primitive,
                    CreativeWorldLayoutPlanPoint point) noexcept {
  bool inside = false;
  std::size_t previous = primitive.pointCount - 1U;
  for (std::size_t current = 0U; current < primitive.pointCount;
       previous = current++) {
    const CreativeWorldLayoutPlanPoint a = primitive.points[current];
    const CreativeWorldLayoutPlanPoint b = primitive.points[previous];
    const bool crosses = (a.z > point.z) != (b.z > point.z);
    if (!crosses) {
      continue;
    }
    const double crossingX =
        (b.x - a.x) * (point.z - a.z) / (b.z - a.z) + a.x;
    if (point.x < crossingX) {
      inside = !inside;
    }
  }
  return inside;
}

double positiveAngle(double radians) noexcept {
  double value = std::fmod(radians, kTwoPi);
  if (value < 0.0) {
    value += kTwoPi;
  }
  return value;
}

bool angleWithinSweep(double angle, double start, double sweep) noexcept {
  if (std::abs(sweep) >= kTwoPi - kGeometryEpsilon) {
    return true;
  }
  if (sweep >= 0.0) {
    return positiveAngle(angle - start) <= sweep + kGeometryEpsilon;
  }
  return positiveAngle(start - angle) <= -sweep + kGeometryEpsilon;
}

CreativeWorldLayoutPlanPoint arcPoint(CreativeWorldLayoutPlanPoint center,
                                      double radius,
                                      double radians) noexcept {
  return {center.x + std::cos(radians) * radius,
          center.z + std::sin(radians) * radius};
}

bool validPrimitive(const CreativeWorldLayoutPlanPrimitive& primitive) noexcept {
  if (primitive.role >= CreativeWorldLayoutPlanRole::Count ||
      primitive.kind >= CreativeWorldLayoutPlanPrimitiveKind::Count ||
      primitive.layer >= CreativeWorldLayoutPlanLayer::Count ||
      !std::isfinite(primitive.widthCells) || primitive.widthCells < 0.0) {
    return false;
  }
  std::size_t requiredPointCount = 0U;
  switch (primitive.kind) {
    case CreativeWorldLayoutPlanPrimitiveKind::Segment:
      requiredPointCount = 2U;
      break;
    case CreativeWorldLayoutPlanPrimitiveKind::Polygon:
      if (primitive.pointCount < 3U ||
          primitive.pointCount > primitive.points.size()) {
        return false;
      }
      requiredPointCount = primitive.pointCount;
      break;
    case CreativeWorldLayoutPlanPrimitiveKind::Circle:
      if (!std::isfinite(primitive.radiusCells) ||
          primitive.radiusCells < 0.0) {
        return false;
      }
      requiredPointCount = 1U;
      break;
    case CreativeWorldLayoutPlanPrimitiveKind::Arc:
      if (!std::isfinite(primitive.radiusCells) ||
          primitive.radiusCells < 0.0 ||
          !std::isfinite(primitive.startRadians) ||
          !std::isfinite(primitive.sweepRadians)) {
        return false;
      }
      requiredPointCount = 1U;
      break;
    case CreativeWorldLayoutPlanPrimitiveKind::Point:
      requiredPointCount = 1U;
      break;
    case CreativeWorldLayoutPlanPrimitiveKind::Count:
      return false;
  }
  if (primitive.pointCount != requiredPointCount) {
    return false;
  }
  for (std::size_t index = 0U; index < requiredPointCount; ++index) {
    if (!finite(primitive.points[index])) {
      return false;
    }
  }
  return true;
}

CreativeWorldLayoutPlanHitTestResult result(bool hit, double distance) noexcept {
  return {
      hit,
      hit ? CreativeWorldLayoutPlanHitTestStatus::Hit
          : CreativeWorldLayoutPlanHitTestStatus::Miss,
      distance,
      hit ? "creative_world_layout_plan_hit_test_hit"
          : "creative_world_layout_plan_hit_test_miss",
  };
}

bool within(double distance, double footprint) noexcept {
  return distance <= footprint + kGeometryEpsilon;
}

}  // namespace

std::string_view toString(CreativeWorldLayoutPlanHitTestStatus status) noexcept {
  switch (status) {
    case CreativeWorldLayoutPlanHitTestStatus::NotRequested:
      return "NotRequested";
    case CreativeWorldLayoutPlanHitTestStatus::InvalidRequest:
      return "InvalidRequest";
    case CreativeWorldLayoutPlanHitTestStatus::InvalidPrimitive:
      return "InvalidPrimitive";
    case CreativeWorldLayoutPlanHitTestStatus::Miss:
      return "Miss";
    case CreativeWorldLayoutPlanHitTestStatus::Hit:
      return "Hit";
  }
  return "Unknown";
}

CreativeWorldLayoutPlanHitTestResult hitTestCreativeWorldLayoutPlanPrimitive(
    const CreativeWorldLayoutPlanPrimitive& primitive,
    CreativeWorldLayoutPlanPoint point, double toleranceCells) noexcept {
  if (!finite(point) || !std::isfinite(toleranceCells) ||
      toleranceCells < 0.0) {
    return {false, CreativeWorldLayoutPlanHitTestStatus::InvalidRequest, 0.0,
            "creative_world_layout_plan_hit_test_request_invalid"};
  }
  if (!validPrimitive(primitive)) {
    return {false, CreativeWorldLayoutPlanHitTestStatus::InvalidPrimitive, 0.0,
            "creative_world_layout_plan_hit_test_primitive_invalid"};
  }

  switch (primitive.kind) {
    case CreativeWorldLayoutPlanPrimitiveKind::Segment: {
      const double distance = pointSegmentDistance(
          point, primitive.points[0], primitive.points[1]);
      const double footprint = toleranceCells + primitive.widthCells * 0.5;
      return result(within(distance, footprint), distance);
    }
    case CreativeWorldLayoutPlanPrimitiveKind::Polygon: {
      double distance = std::numeric_limits<double>::infinity();
      for (std::size_t index = 0U; index < primitive.pointCount; ++index) {
        distance = std::min(
            distance,
            pointSegmentDistance(
                point, primitive.points[index],
                primitive.points[(index + 1U) % primitive.pointCount]));
      }
      if (pointInPolygon(primitive, point)) {
        distance = 0.0;
      }
      return result(within(distance, toleranceCells), distance);
    }
    case CreativeWorldLayoutPlanPrimitiveKind::Circle: {
      const double centerDistance =
          std::sqrt(squaredDistance(point, primitive.points[0]));
      const double distance =
          std::max(0.0, centerDistance - primitive.radiusCells);
      return result(within(distance, toleranceCells), distance);
    }
    case CreativeWorldLayoutPlanPrimitiveKind::Arc: {
      const CreativeWorldLayoutPlanPoint center = primitive.points[0];
      const double centerDistance = std::sqrt(squaredDistance(point, center));
      const double angle = std::atan2(point.z - center.z, point.x - center.x);
      double distance = std::numeric_limits<double>::infinity();
      if (angleWithinSweep(angle, primitive.startRadians,
                           primitive.sweepRadians)) {
        distance = std::abs(centerDistance - primitive.radiusCells);
      }
      distance = std::min(
          distance,
          std::sqrt(squaredDistance(
              point, arcPoint(center, primitive.radiusCells,
                              primitive.startRadians))));
      distance = std::min(
          distance,
          std::sqrt(squaredDistance(
              point, arcPoint(center, primitive.radiusCells,
                              primitive.startRadians +
                                  primitive.sweepRadians))));
      return result(within(distance, toleranceCells), distance);
    }
    case CreativeWorldLayoutPlanPrimitiveKind::Point: {
      const double distance =
          std::sqrt(squaredDistance(point, primitive.points[0]));
      return result(within(distance, toleranceCells), distance);
    }
    case CreativeWorldLayoutPlanPrimitiveKind::Count:
      break;
  }
  return {false, CreativeWorldLayoutPlanHitTestStatus::InvalidPrimitive, 0.0,
          "creative_world_layout_plan_hit_test_primitive_invalid"};
}

}  // namespace iggy3d::creative
