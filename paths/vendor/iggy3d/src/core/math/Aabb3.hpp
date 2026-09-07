#pragma once

#include "core/math/Ray3.hpp"
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

struct AabbRayHit {
  bool hit = false;
  float distanceMeters = 0.0F;
  float exitDistanceMeters = 0.0F;
  Vec3 pointMeters{};
  bool startInside = false;
};

// Ray-vs-AABB slab test. `ray.direction` need not be unit length; it is normalized internally so
// returned distances are in meters. No hit for invalid bounds/input, degenerate direction,
// non-finite/negative maxDistanceMeters, or boxes outside the forward ray interval.
[[nodiscard]] AabbRayHit intersectsRay(const Aabb3& bounds,
                                       Ray3 ray,
                                       float maxDistanceMeters);

}  // namespace iggy3d
