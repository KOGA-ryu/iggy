#include "app/iggy3d/creative/world/WorldLayoutPlanHitTest.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
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

struct RegionRect {
  double minimumX = 0.0;
  double minimumZ = 0.0;
  double maximumX = 0.0;
  double maximumZ = 0.0;
};

RegionRect normalizedRect(CreativeWorldLayoutPlanPoint first,
                          CreativeWorldLayoutPlanPoint second) noexcept {
  return {
      std::min(first.x, second.x),
      std::min(first.z, second.z),
      std::max(first.x, second.x),
      std::max(first.z, second.z),
  };
}

RegionRect expandedRect(RegionRect rect, double amount) noexcept {
  rect.minimumX -= amount;
  rect.minimumZ -= amount;
  rect.maximumX += amount;
  rect.maximumZ += amount;
  return rect;
}

bool rectContains(RegionRect rect,
                  CreativeWorldLayoutPlanPoint point) noexcept {
  return point.x >= rect.minimumX - kGeometryEpsilon &&
         point.x <= rect.maximumX + kGeometryEpsilon &&
         point.z >= rect.minimumZ - kGeometryEpsilon &&
         point.z <= rect.maximumZ + kGeometryEpsilon;
}

bool rectContains(RegionRect outer, RegionRect inner) noexcept {
  return inner.minimumX >= outer.minimumX - kGeometryEpsilon &&
         inner.maximumX <= outer.maximumX + kGeometryEpsilon &&
         inner.minimumZ >= outer.minimumZ - kGeometryEpsilon &&
         inner.maximumZ <= outer.maximumZ + kGeometryEpsilon;
}

bool segmentIntersectsRect(CreativeWorldLayoutPlanPoint start,
                           CreativeWorldLayoutPlanPoint end,
                           RegionRect rect) noexcept {
  if (rectContains(rect, start) || rectContains(rect, end)) {
    return true;
  }
  const double dx = end.x - start.x;
  const double dz = end.z - start.z;
  double first = 0.0;
  double last = 1.0;
  const auto clip = [&](double direction, double distance) {
    if (std::abs(direction) <= kGeometryEpsilon) {
      return distance >= -kGeometryEpsilon;
    }
    const double value = distance / direction;
    if (direction < 0.0) {
      if (value > last) {
        return false;
      }
      first = std::max(first, value);
    } else {
      if (value < first) {
        return false;
      }
      last = std::min(last, value);
    }
    return first <= last + kGeometryEpsilon;
  };
  return clip(-dx, start.x - rect.minimumX) &&
         clip(dx, rect.maximumX - start.x) &&
         clip(-dz, start.z - rect.minimumZ) &&
         clip(dz, rect.maximumZ - start.z);
}

bool polygonIntersectsRect(const CreativeWorldLayoutPlanPrimitive& primitive,
                           RegionRect rect) noexcept {
  for (std::size_t index = 0U; index < primitive.pointCount; ++index) {
    if (rectContains(rect, primitive.points[index]) ||
        segmentIntersectsRect(
            primitive.points[index],
            primitive.points[(index + 1U) % primitive.pointCount], rect)) {
      return true;
    }
  }
  const CreativeWorldLayoutPlanPoint corners[] = {
      {rect.minimumX, rect.minimumZ},
      {rect.maximumX, rect.minimumZ},
      {rect.maximumX, rect.maximumZ},
      {rect.minimumX, rect.maximumZ},
  };
  return std::any_of(std::begin(corners), std::end(corners),
                     [&](CreativeWorldLayoutPlanPoint corner) {
                       return pointInPolygon(primitive, corner);
                     });
}

bool circleIntersectsRect(CreativeWorldLayoutPlanPoint center, double radius,
                          RegionRect rect) noexcept {
  const double closestX =
      std::clamp(center.x, rect.minimumX, rect.maximumX);
  const double closestZ =
      std::clamp(center.z, rect.minimumZ, rect.maximumZ);
  const double dx = center.x - closestX;
  const double dz = center.z - closestZ;
  return dx * dx + dz * dz <= radius * radius + kGeometryEpsilon;
}

bool arcIntersectsRect(const CreativeWorldLayoutPlanPrimitive& primitive,
                       RegionRect rect) noexcept {
  const CreativeWorldLayoutPlanPoint center = primitive.points[0];
  const double radius = primitive.radiusCells;
  const auto pointOnArc = [&](CreativeWorldLayoutPlanPoint point) {
    const double angle = std::atan2(point.z - center.z, point.x - center.x);
    return angleWithinSweep(angle, primitive.startRadians,
                            primitive.sweepRadians);
  };
  const CreativeWorldLayoutPlanPoint start =
      arcPoint(center, radius, primitive.startRadians);
  const CreativeWorldLayoutPlanPoint end = arcPoint(
      center, radius, primitive.startRadians + primitive.sweepRadians);
  if (rectContains(rect, start) || rectContains(rect, end)) {
    return true;
  }
  if (radius <= kGeometryEpsilon) {
    return rectContains(rect, center);
  }

  const auto intersectsVertical = [&](double x) {
    const double offset = x - center.x;
    if (std::abs(offset) > radius + kGeometryEpsilon) {
      return false;
    }
    const double remainder = std::max(0.0, radius * radius - offset * offset);
    const double dz = std::sqrt(remainder);
    const CreativeWorldLayoutPlanPoint candidates[] = {
        {x, center.z - dz}, {x, center.z + dz}};
    return std::any_of(std::begin(candidates), std::end(candidates),
                       [&](CreativeWorldLayoutPlanPoint point) {
                         return point.z >= rect.minimumZ - kGeometryEpsilon &&
                                point.z <= rect.maximumZ + kGeometryEpsilon &&
                                pointOnArc(point);
                       });
  };
  const auto intersectsHorizontal = [&](double z) {
    const double offset = z - center.z;
    if (std::abs(offset) > radius + kGeometryEpsilon) {
      return false;
    }
    const double remainder = std::max(0.0, radius * radius - offset * offset);
    const double dx = std::sqrt(remainder);
    const CreativeWorldLayoutPlanPoint candidates[] = {
        {center.x - dx, z}, {center.x + dx, z}};
    return std::any_of(std::begin(candidates), std::end(candidates),
                       [&](CreativeWorldLayoutPlanPoint point) {
                         return point.x >= rect.minimumX - kGeometryEpsilon &&
                                point.x <= rect.maximumX + kGeometryEpsilon &&
                                pointOnArc(point);
                       });
  };
  return intersectsVertical(rect.minimumX) ||
         intersectsVertical(rect.maximumX) ||
         intersectsHorizontal(rect.minimumZ) ||
         intersectsHorizontal(rect.maximumZ);
}

void includePoint(RegionRect& bounds, bool& initialized,
                  CreativeWorldLayoutPlanPoint point) noexcept {
  if (!initialized) {
    bounds = {point.x, point.z, point.x, point.z};
    initialized = true;
    return;
  }
  bounds.minimumX = std::min(bounds.minimumX, point.x);
  bounds.minimumZ = std::min(bounds.minimumZ, point.z);
  bounds.maximumX = std::max(bounds.maximumX, point.x);
  bounds.maximumZ = std::max(bounds.maximumZ, point.z);
}

RegionRect primitiveBounds(
    const CreativeWorldLayoutPlanPrimitive& primitive,
    double toleranceCells) noexcept {
  RegionRect bounds;
  bool initialized = false;
  switch (primitive.kind) {
    case CreativeWorldLayoutPlanPrimitiveKind::Segment:
    case CreativeWorldLayoutPlanPrimitiveKind::Polygon:
      for (std::size_t index = 0U; index < primitive.pointCount; ++index) {
        includePoint(bounds, initialized, primitive.points[index]);
      }
      break;
    case CreativeWorldLayoutPlanPrimitiveKind::Circle: {
      const double radius = primitive.radiusCells;
      bounds = {primitive.points[0].x - radius,
                primitive.points[0].z - radius,
                primitive.points[0].x + radius,
                primitive.points[0].z + radius};
      initialized = true;
      break;
    }
    case CreativeWorldLayoutPlanPrimitiveKind::Arc: {
      includePoint(bounds, initialized,
                   arcPoint(primitive.points[0], primitive.radiusCells,
                            primitive.startRadians));
      includePoint(bounds, initialized,
                   arcPoint(primitive.points[0], primitive.radiusCells,
                            primitive.startRadians + primitive.sweepRadians));
      constexpr double extrema[] = {0.0, kPi * 0.5, kPi, kPi * 1.5};
      for (double angle : extrema) {
        if (angleWithinSweep(angle, primitive.startRadians,
                             primitive.sweepRadians)) {
          includePoint(bounds, initialized,
                       arcPoint(primitive.points[0], primitive.radiusCells,
                                angle));
        }
      }
      break;
    }
    case CreativeWorldLayoutPlanPrimitiveKind::Point:
      includePoint(bounds, initialized, primitive.points[0]);
      break;
    case CreativeWorldLayoutPlanPrimitiveKind::Count:
      break;
  }
  const double width =
      primitive.kind == CreativeWorldLayoutPlanPrimitiveKind::Segment
          ? primitive.widthCells * 0.5
          : 0.0;
  return expandedRect(bounds, toleranceCells + width);
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

CreativeWorldLayoutPlanHitTestResult
selectCreativeWorldLayoutPlanPrimitiveInRegion(
    const CreativeWorldLayoutPlanPrimitive& primitive,
    CreativeWorldLayoutPlanPoint first,
    CreativeWorldLayoutPlanPoint second,
    CreativeWorldLayoutPlanRegionMode mode,
    double toleranceCells) noexcept {
  if (!finite(first) || !finite(second) || !std::isfinite(toleranceCells) ||
      toleranceCells < 0.0 || mode >= CreativeWorldLayoutPlanRegionMode::Count) {
    return {false, CreativeWorldLayoutPlanHitTestStatus::InvalidRequest, 0.0,
            "creative_world_layout_plan_region_request_invalid"};
  }
  if (!validPrimitive(primitive)) {
    return {false, CreativeWorldLayoutPlanHitTestStatus::InvalidPrimitive, 0.0,
            "creative_world_layout_plan_region_primitive_invalid"};
  }

  const RegionRect region = normalizedRect(first, second);
  if (mode == CreativeWorldLayoutPlanRegionMode::Window) {
    const bool selected =
        rectContains(region, primitiveBounds(primitive, toleranceCells));
    return {selected,
            selected ? CreativeWorldLayoutPlanHitTestStatus::Hit
                     : CreativeWorldLayoutPlanHitTestStatus::Miss,
            0.0,
            selected ? "creative_world_layout_plan_region_window_hit"
                     : "creative_world_layout_plan_region_window_miss"};
  }

  bool selected = false;
  switch (primitive.kind) {
    case CreativeWorldLayoutPlanPrimitiveKind::Segment:
      selected = segmentIntersectsRect(
          primitive.points[0], primitive.points[1],
          expandedRect(region,
                       toleranceCells + primitive.widthCells * 0.5));
      break;
    case CreativeWorldLayoutPlanPrimitiveKind::Polygon:
      selected = polygonIntersectsRect(
          primitive, expandedRect(region, toleranceCells));
      break;
    case CreativeWorldLayoutPlanPrimitiveKind::Circle:
      selected = circleIntersectsRect(
          primitive.points[0], primitive.radiusCells,
          expandedRect(region, toleranceCells));
      break;
    case CreativeWorldLayoutPlanPrimitiveKind::Arc:
      selected = arcIntersectsRect(
          primitive, expandedRect(region, toleranceCells));
      break;
    case CreativeWorldLayoutPlanPrimitiveKind::Point:
      selected = rectContains(expandedRect(region, toleranceCells),
                              primitive.points[0]);
      break;
    case CreativeWorldLayoutPlanPrimitiveKind::Count:
      break;
  }
  return {selected,
          selected ? CreativeWorldLayoutPlanHitTestStatus::Hit
                   : CreativeWorldLayoutPlanHitTestStatus::Miss,
          0.0,
          selected ? "creative_world_layout_plan_region_crossing_hit"
                   : "creative_world_layout_plan_region_crossing_miss"};
}

}  // namespace iggy3d::creative
