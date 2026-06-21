#include "core/math/Transform3.hpp"

namespace iggy3d {

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

Vec3 transformPoint(const Transform3& transform, Vec3 localPoint) {
  return transform.position +
         Vec3{localPoint.x * transform.scale.x, localPoint.y * transform.scale.y,
              localPoint.z * transform.scale.z};
}

}  // namespace iggy3d
