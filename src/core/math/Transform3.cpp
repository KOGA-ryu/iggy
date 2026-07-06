#include "core/math/Transform3.hpp"

#include "core/math/EulerRotation.hpp"

namespace iggy3d {

namespace {

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
  return transform.position + rotateEulerXyz(scaleLocalPoint(transform, localPoint),
                                             transform.rotationEulerRadians);
}

Vec3 transformPoint(const Transform3& transform, Vec3 localPoint) {
  return transformPointScaleTranslate(transform, localPoint);
}

}  // namespace iggy3d
