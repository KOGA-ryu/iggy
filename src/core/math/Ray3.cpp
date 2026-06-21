#include "core/math/Ray3.hpp"

namespace iggy3d {

Ray3 makeRay3(Vec3 origin, Vec3 direction) {
  return {origin, direction};
}

bool isFinite(const Ray3& ray) {
  return isFinite(ray.origin) && isFinite(ray.direction);
}

bool isValid(const Ray3& ray) {
  return isFinite(ray) && lengthSquared(ray.direction) > 0.0F;
}

Vec3 pointAt(const Ray3& ray, float t) {
  return ray.origin + ray.direction * t;
}

}  // namespace iggy3d
