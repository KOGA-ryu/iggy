#pragma once

#include "core/math/Ray3.hpp"

namespace iggy3d {

struct Plane {
  Vec3 normal;
  float distance = 0.0F;
};

struct RayPlaneHit {
  bool hit = false;
  float t = 0.0F;
  Vec3 point;
};

Plane makePlane(Vec3 normal, float distance);
Plane planeFromPointNormal(Vec3 point, Vec3 normal);
bool isFinite(const Plane& plane);
bool isValid(const Plane& plane);
float signedDistance(const Plane& plane, Vec3 point);
RayPlaneHit intersectRayPlane(const Ray3& ray, const Plane& plane);

}  // namespace iggy3d
