#include "app/iggy3d/creative/Geometry.hpp"

#include <cmath>
#include <limits>

namespace iggy3d::creative {

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
