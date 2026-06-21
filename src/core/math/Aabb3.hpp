#pragma once

#include "core/math/Vec3.hpp"

namespace iggy3d {

struct Aabb3 {
  Vec3 min;
  Vec3 max;
};

Aabb3 makeAabb3(Vec3 min, Vec3 max);
Aabb3 aabbFromCenterExtents(Vec3 center, Vec3 extents);
bool isFinite(const Aabb3& bounds);
bool isValid(const Aabb3& bounds);
Vec3 center(const Aabb3& bounds);
Vec3 extents(const Aabb3& bounds);
bool contains(const Aabb3& bounds, Vec3 point);
bool intersects(const Aabb3& a, const Aabb3& b);
Vec3 closestPoint(const Aabb3& bounds, Vec3 point);

}  // namespace iggy3d
