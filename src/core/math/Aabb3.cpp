#include "core/math/Aabb3.hpp"

#include <algorithm>
#include <cmath>

namespace iggy3d {

Aabb3 makeAabb3(Vec3 min, Vec3 max) {
  return {min, max};
}

Aabb3 aabbFromCenterExtents(Vec3 centerValue, Vec3 extentsValue) {
  return {centerValue - extentsValue, centerValue + extentsValue};
}

bool isFinite(const Aabb3& bounds) {
  return isFinite(bounds.min) && isFinite(bounds.max);
}

bool isValid(const Aabb3& bounds) {
  return isFinite(bounds) && bounds.min.x <= bounds.max.x && bounds.min.y <= bounds.max.y &&
         bounds.min.z <= bounds.max.z;
}

Vec3 center(const Aabb3& bounds) {
  return (bounds.min + bounds.max) * 0.5F;
}

Vec3 extents(const Aabb3& bounds) {
  return (bounds.max - bounds.min) * 0.5F;
}

bool contains(const Aabb3& bounds, Vec3 point) {
  return isValid(bounds) && point.x >= bounds.min.x && point.x <= bounds.max.x &&
         point.y >= bounds.min.y && point.y <= bounds.max.y && point.z >= bounds.min.z &&
         point.z <= bounds.max.z;
}

bool intersects(const Aabb3& a, const Aabb3& b) {
  if (!isValid(a) || !isValid(b)) {
    return false;
  }
  return a.min.x <= b.max.x && a.max.x >= b.min.x && a.min.y <= b.max.y &&
         a.max.y >= b.min.y && a.min.z <= b.max.z && a.max.z >= b.min.z;
}

Vec3 closestPoint(const Aabb3& bounds, Vec3 point) {
  return {std::clamp(point.x, bounds.min.x, bounds.max.x),
          std::clamp(point.y, bounds.min.y, bounds.max.y),
          std::clamp(point.z, bounds.min.z, bounds.max.z)};
}

AabbRayHit intersectsRay(const Aabb3& bounds,
                         Ray3 ray,
                         float maxDistanceMeters) {
  AabbRayHit result;
  if (!isValid(bounds) || !isFinite(ray.origin) || !isFinite(ray.direction) ||
      !std::isfinite(maxDistanceMeters) || maxDistanceMeters < 0.0F) {
    return result;
  }

  Vec3 unitDirection;
  if (!tryNormalize(ray.direction, unitDirection)) {
    return result;
  }

  float tEnter = 0.0F;
  float tExit = maxDistanceMeters;
  for (int axis = 0; axis < 3; ++axis) {
    const float origin =
        axis == 0 ? ray.origin.x : (axis == 1 ? ray.origin.y : ray.origin.z);
    const float direction = axis == 0 ? unitDirection.x
                                      : (axis == 1 ? unitDirection.y
                                                   : unitDirection.z);
    const float minValue =
        axis == 0 ? bounds.min.x : (axis == 1 ? bounds.min.y : bounds.min.z);
    const float maxValue =
        axis == 0 ? bounds.max.x : (axis == 1 ? bounds.max.y : bounds.max.z);

    constexpr float kParallelEpsilon = 1.0e-6F;
    if (std::fabs(direction) <= kParallelEpsilon) {
      if (origin < minValue || origin > maxValue) {
        return result;
      }
      continue;
    }

    const float inverseDirection = 1.0F / direction;
    float nearDistance = (minValue - origin) * inverseDirection;
    float farDistance = (maxValue - origin) * inverseDirection;
    if (nearDistance > farDistance) {
      std::swap(nearDistance, farDistance);
    }
    tEnter = std::max(tEnter, nearDistance);
    tExit = std::min(tExit, farDistance);
    if (tEnter > tExit) {
      return result;
    }
  }

  result.hit = true;
  result.distanceMeters = tEnter;
  result.exitDistanceMeters = tExit;
  result.pointMeters = ray.origin + unitDirection * tEnter;
  result.startInside = contains(bounds, ray.origin);
  return result;
}

}  // namespace iggy3d
