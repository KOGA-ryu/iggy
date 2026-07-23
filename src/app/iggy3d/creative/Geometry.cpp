#include "app/iggy3d/creative/Geometry.hpp"

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace iggy3d::creative {
CreativeVec3 rotateCreativeVectorEulerXyz(CreativeVec3 vector,
                                          CreativeVec3 radians) noexcept {
  const double cx = std::cos(radians.x);
  const double sx = std::sin(radians.x);
  const double cy = std::cos(radians.y);
  const double sy = std::sin(radians.y);
  const double cz = std::cos(radians.z);
  const double sz = std::sin(radians.z);
  const double y1 = vector.y * cx - vector.z * sx;
  const double z1 = vector.y * sx + vector.z * cx;
  const double x2 = vector.x * cy + z1 * sy;
  const double z2 = -vector.x * sy + z1 * cy;
  return {x2 * cz - y1 * sz, x2 * sz + y1 * cz, z2};
}

CreativeVec3 creativeEulerXyzFromBasis(CreativeVec3 basisX,
                                       CreativeVec3 basisY,
                                       CreativeVec3 basisZ) noexcept {
  if (!isFiniteCreativeVec3(basisX) || !isFiniteCreativeVec3(basisY) ||
      !isFiniteCreativeVec3(basisZ)) {
    const double invalid = std::numeric_limits<double>::quiet_NaN();
    return {invalid, invalid, invalid};
  }
  const double sinY = std::clamp(-basisX.z, -1.0, 1.0);
  CreativeVec3 result;
  result.y = std::asin(sinY);
  const double cosY = std::cos(result.y);
  if (std::fabs(cosY) > 1.0e-10) {
    result.x = std::atan2(basisY.z, basisZ.z);
    result.z = std::atan2(basisX.y, basisX.x);
  } else {
    result.x = sinY > 0.0 ? std::atan2(basisY.x, basisY.y)
                          : std::atan2(-basisY.x, basisY.y);
    result.z = 0.0;
  }
  result.x = std::remainder(result.x, std::numbers::pi * 2.0);
  result.y = std::remainder(result.y, std::numbers::pi * 2.0);
  result.z = std::remainder(result.z, std::numbers::pi * 2.0);
  return result;
}

bool isValidCreativeAxis3(CreativeAxis3 axis) noexcept {
  return static_cast<std::uint8_t>(axis) <
         static_cast<std::uint8_t>(CreativeAxis3::Count);
}

std::string_view toString(CreativeAxis3 axis) noexcept {
  switch (axis) {
    case CreativeAxis3::X: return "X";
    case CreativeAxis3::Y: return "Y";
    case CreativeAxis3::Z: return "Z";
    case CreativeAxis3::Count: break;
  }
  return "INVALID";
}

CreativeVec3 rotateCreativeVectorAxisAngle(CreativeVec3 vector,
                                           CreativeAxis3 axis,
                                           double radians) noexcept {
  const double cosine = std::cos(radians);
  const double sine = std::sin(radians);
  switch (axis) {
    case CreativeAxis3::X:
      return {vector.x, vector.y * cosine - vector.z * sine,
              vector.y * sine + vector.z * cosine};
    case CreativeAxis3::Y:
      return {vector.x * cosine + vector.z * sine, vector.y,
              -vector.x * sine + vector.z * cosine};
    case CreativeAxis3::Z:
      return {vector.x * cosine - vector.y * sine,
              vector.x * sine + vector.y * cosine, vector.z};
    case CreativeAxis3::Count:
      return {};
  }
  return {};
}

CreativeVec3 rotateCreativeVectorAroundAxis(CreativeVec3 vector,
                                            CreativeVec3 axis,
                                            double radians) noexcept {
  const double lengthSquared =
      axis.x * axis.x + axis.y * axis.y + axis.z * axis.z;
  if (!isFiniteCreativeVec3(vector) || !isFiniteCreativeVec3(axis) ||
      !std::isfinite(radians) || !std::isfinite(lengthSquared) ||
      lengthSquared <= 1.0e-24) {
    const double invalid = std::numeric_limits<double>::quiet_NaN();
    return {invalid, invalid, invalid};
  }
  const double inverseLength = 1.0 / std::sqrt(lengthSquared);
  axis.x *= inverseLength;
  axis.y *= inverseLength;
  axis.z *= inverseLength;
  const double cosine = std::cos(radians);
  const double sine = std::sin(radians);
  const double oneMinusCosine = 1.0 - cosine;
  const double projection =
      axis.x * vector.x + axis.y * vector.y + axis.z * vector.z;
  const CreativeVec3 cross{
      axis.y * vector.z - axis.z * vector.y,
      axis.z * vector.x - axis.x * vector.z,
      axis.x * vector.y - axis.y * vector.x,
  };
  return {
      vector.x * cosine + cross.x * sine +
          axis.x * projection * oneMinusCosine,
      vector.y * cosine + cross.y * sine +
          axis.y * projection * oneMinusCosine,
      vector.z * cosine + cross.z * sine +
          axis.z * projection * oneMinusCosine,
  };
}

CreativeVec3 composeCreativeWorldAxisRotation(CreativeVec3 eulerRadians,
                                              CreativeAxis3 axis,
                                              double radians) noexcept {
  if (radians == 0.0) {
    return eulerRadians;
  }
  const CreativeVec3 basisX = rotateCreativeVectorAxisAngle(
      rotateCreativeVectorEulerXyz({1.0, 0.0, 0.0}, eulerRadians), axis,
      radians);
  const CreativeVec3 basisY = rotateCreativeVectorAxisAngle(
      rotateCreativeVectorEulerXyz({0.0, 1.0, 0.0}, eulerRadians), axis,
      radians);
  const CreativeVec3 basisZ = rotateCreativeVectorAxisAngle(
      rotateCreativeVectorEulerXyz({0.0, 0.0, 1.0}, eulerRadians), axis,
      radians);
  return creativeEulerXyzFromBasis(basisX, basisY, basisZ);
}

CreativeVec3 composeCreativeWorldAxisRotation(CreativeVec3 eulerRadians,
                                              CreativeVec3 axis,
                                              double radians) noexcept {
  if (radians == 0.0) {
    return eulerRadians;
  }
  const CreativeVec3 basisX = rotateCreativeVectorAroundAxis(
      rotateCreativeVectorEulerXyz({1.0, 0.0, 0.0}, eulerRadians), axis,
      radians);
  const CreativeVec3 basisY = rotateCreativeVectorAroundAxis(
      rotateCreativeVectorEulerXyz({0.0, 1.0, 0.0}, eulerRadians), axis,
      radians);
  const CreativeVec3 basisZ = rotateCreativeVectorAroundAxis(
      rotateCreativeVectorEulerXyz({0.0, 0.0, 1.0}, eulerRadians), axis,
      radians);
  return creativeEulerXyzFromBasis(basisX, basisY, basisZ);
}

double creativeSquaredDistanceFromAxis(CreativeVec3 point,
                                       CreativeVec3 axisPoint,
                                       CreativeAxis3 axis) noexcept {
  const CreativeVec3 delta{point.x - axisPoint.x, point.y - axisPoint.y,
                           point.z - axisPoint.z};
  switch (axis) {
    case CreativeAxis3::X:
      return delta.y * delta.y + delta.z * delta.z;
    case CreativeAxis3::Y:
      return delta.x * delta.x + delta.z * delta.z;
    case CreativeAxis3::Z:
      return delta.x * delta.x + delta.y * delta.y;
    case CreativeAxis3::Count:
      return std::numeric_limits<double>::quiet_NaN();
  }
  return std::numeric_limits<double>::quiet_NaN();
}

bool isFiniteCreativeVec3(CreativeVec3 value) noexcept {
  return std::isfinite(value.x) && std::isfinite(value.y) &&
         std::isfinite(value.z);
}

bool creativeVec3ExactlyEqual(CreativeVec3 lhs, CreativeVec3 rhs) noexcept {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

bool creativeBoundsExactlyEqual(CreativeBounds lhs,
                                CreativeBounds rhs) noexcept {
  return creativeVec3ExactlyEqual(lhs.min, rhs.min) &&
         creativeVec3ExactlyEqual(lhs.max, rhs.max);
}

bool isPositiveCreativeVec3(CreativeVec3 value) noexcept {
  return isFiniteCreativeVec3(value) && value.x > 0.0 && value.y > 0.0 &&
         value.z > 0.0;
}

CreativeBoundsMetrics measureCreativeBounds(CreativeBounds bounds) noexcept {
  CreativeBoundsMetrics result;
  if (!isFiniteCreativeVec3(bounds.min) ||
      !isFiniteCreativeVec3(bounds.max)) {
    result.status = CreativeGeometryStatus::NonFiniteBounds;
    return result;
  }
  if (bounds.min.x > bounds.max.x || bounds.min.y > bounds.max.y ||
      bounds.min.z > bounds.max.z) {
    result.status = CreativeGeometryStatus::ReversedBounds;
    return result;
  }

  result.size = {
      bounds.max.x - bounds.min.x,
      bounds.max.y - bounds.min.y,
      bounds.max.z - bounds.min.z,
  };
  result.center = {
      bounds.min.x + result.size.x * 0.5,
      bounds.min.y + result.size.y * 0.5,
      bounds.min.z + result.size.z * 0.5,
  };
  if (!isFiniteCreativeVec3(result.size) ||
      !isFiniteCreativeVec3(result.center)) {
    result.center = {};
    result.size = {};
    result.status = CreativeGeometryStatus::ArithmeticOverflow;
    return result;
  }

  result.status = CreativeGeometryStatus::Valid;
  result.valid = true;
  return result;
}

CreativeObjectWorldExtent resolveCreativeObjectWorldExtent(
    const CreativeObject& object) noexcept {
  CreativeObjectWorldExtent extent;
  if (objectHasBounds(object.kind)) {
    const CreativeTransformedBounds resolved =
        resolveCreativeObjectBounds(object);
    if (!resolved.valid) {
      return extent;
    }
    extent.min = resolved.worldBounds.min;
    extent.max = resolved.worldBounds.max;
    extent.valid = true;
    return extent;
  }
  if (!object.pathPoints.empty()) {
    for (const CreativePathPoint& point : object.pathPoints) {
      if (!isFiniteCreativeVec3(point.position)) {
        return {};
      }
      if (!extent.valid) {
        extent.min = point.position;
        extent.max = point.position;
        extent.valid = true;
        continue;
      }
      extent.min.x = std::min(extent.min.x, point.position.x);
      extent.min.y = std::min(extent.min.y, point.position.y);
      extent.min.z = std::min(extent.min.z, point.position.z);
      extent.max.x = std::max(extent.max.x, point.position.x);
      extent.max.y = std::max(extent.max.y, point.position.y);
      extent.max.z = std::max(extent.max.z, point.position.z);
    }
    return extent;
  }
  if (!objectHasTransform(object.kind) ||
      !isFiniteCreativeVec3(object.transform.position)) {
    return extent;
  }
  extent.min = object.transform.position;
  extent.max = object.transform.position;
  extent.valid = true;
  return extent;
}

CreativeVec3 creativeVec3FromCore(iggy3d::Vec3 value) noexcept {
  return {static_cast<double>(value.x), static_cast<double>(value.y),
          static_cast<double>(value.z)};
}

CreativeCoreVec3Conversion creativeVec3ToCoreChecked(
    CreativeVec3 value) noexcept {
  CreativeCoreVec3Conversion result;
  if (!isFiniteCreativeVec3(value)) {
    result.status = CreativeGeometryStatus::NonFiniteVector;
    return result;
  }

  constexpr double kMaximumCoreComponent =
      static_cast<double>(std::numeric_limits<float>::max());
  if (std::fabs(value.x) > kMaximumCoreComponent ||
      std::fabs(value.y) > kMaximumCoreComponent ||
      std::fabs(value.z) > kMaximumCoreComponent) {
    result.status = CreativeGeometryStatus::OutsideCoreFloatRange;
    return result;
  }

  result.value = {static_cast<float>(value.x), static_cast<float>(value.y),
                  static_cast<float>(value.z)};
  if (!iggy3d::isFinite(result.value)) {
    result.value = {};
    result.status = CreativeGeometryStatus::OutsideCoreFloatRange;
    return result;
  }

  result.status = CreativeGeometryStatus::Valid;
  result.converted = true;
  return result;
}

}  // namespace iggy3d::creative
