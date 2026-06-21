#include "core/math/Plane.hpp"

#include <cmath>

namespace iggy3d {

Plane makePlane(Vec3 normal, float distance) {
  return {normal, distance};
}

Plane planeFromPointNormal(Vec3 point, Vec3 normal) {
  return {normal, -dot(normal, point)};
}

bool isFinite(const Plane& plane) {
  return isFinite(plane.normal) && std::isfinite(plane.distance);
}

bool isValid(const Plane& plane) {
  return isFinite(plane) && lengthSquared(plane.normal) > 0.0F;
}

float signedDistance(const Plane& plane, Vec3 point) {
  return dot(plane.normal, point) + plane.distance;
}

RayPlaneHit intersectRayPlane(const Ray3& ray, const Plane& plane) {
  const float denominator = dot(plane.normal, ray.direction);
  if (std::fabs(denominator) <= 0.000001F) {
    return {};
  }
  const float t = -signedDistance(plane, ray.origin) / denominator;
  if (t < 0.0F) {
    return {};
  }
  return {true, t, pointAt(ray, t)};
}

}  // namespace iggy3d
