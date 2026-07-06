#pragma once

#include "core/math/Vec3.hpp"

namespace iggy3d {

struct Transform3 {
  Vec3 position;
  Vec3 rotationEulerRadians;
  Vec3 scale = {1.0F, 1.0F, 1.0F};
};

Transform3 identityTransform3();
bool isFinite(const Transform3& transform);
bool hasPositiveFiniteScale(const Transform3& transform);
Vec3 transformPointScaleTranslate(const Transform3& transform, Vec3 localPoint);
Vec3 transformPointTrs(const Transform3& transform, Vec3 localPoint);
Vec3 transformPoint(const Transform3& transform, Vec3 localPoint);

}  // namespace iggy3d
