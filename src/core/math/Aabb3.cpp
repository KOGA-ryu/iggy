#include "core/math/Aabb3.hpp"

#include <algorithm>

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

}  // namespace iggy3d
