#pragma once

#include "core/math/Vec3.hpp"

namespace iggy3d {

struct Ray3 {
  Vec3 origin;
  Vec3 direction;
};

Ray3 makeRay3(Vec3 origin, Vec3 direction);
bool isFinite(const Ray3& ray);
bool isValid(const Ray3& ray);
Vec3 pointAt(const Ray3& ray, float t);

}  // namespace iggy3d
