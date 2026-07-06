#include "core/math/Transform3.hpp"

#include <cmath>

namespace iggy3d {

namespace {

Vec3 rotateEuler(Vec3 p, Vec3 eulerRadians) {
  const float cx = std::cos(eulerRadians.x);
  const float sx = std::sin(eulerRadians.x);
  const float cy = std::cos(eulerRadians.y);
  const float sy = std::sin(eulerRadians.y);
  const float cz = std::cos(eulerRadians.z);
  const float sz = std::sin(eulerRadians.z);

  const float y1 = p.y * cx - p.z * sx;
  const float z1 = p.y * sx + p.z * cx;
  const float x1 = p.x;
  const float x2 = x1 * cy + z1 * sy;
  const float z2 = -x1 * sy + z1 * cy;
  const float y2 = y1;
  const float x3 = x2 * cz - y2 * sz;
  const float y3 = x2 * sz + y2 * cz;
  const float z3 = z2;
  return Vec3{x3, y3, z3};
}

Vec3 scaleLocalPoint(const Transform3& transform, Vec3 localPoint) {
  return {localPoint.x * transform.scale.x, localPoint.y * transform.scale.y,
          localPoint.z * transform.scale.z};
}

}  // namespace

Transform3 identityTransform3() {
  return {};
}

bool isFinite(const Transform3& transform) {
  return isFinite(transform.position) && isFinite(transform.rotationEulerRadians) &&
         isFinite(transform.scale);
}

bool hasPositiveFiniteScale(const Transform3& transform) {
  return isFinite(transform.scale) && transform.scale.x > 0.0F && transform.scale.y > 0.0F &&
         transform.scale.z > 0.0F;
}

Vec3 transformPointScaleTranslate(const Transform3& transform, Vec3 localPoint) {
  return transform.position + scaleLocalPoint(transform, localPoint);
}

Vec3 transformPointTrs(const Transform3& transform, Vec3 localPoint) {
  return transform.position + rotateEuler(scaleLocalPoint(transform, localPoint),
                                          transform.rotationEulerRadians);
}

Vec3 transformPoint(const Transform3& transform, Vec3 localPoint) {
  return transformPointScaleTranslate(transform, localPoint);
}

}  // namespace iggy3d
