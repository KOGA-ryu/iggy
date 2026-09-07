#include "core/math/EulerRotation.hpp"

#include <cmath>

namespace iggy3d {

Vec3 rotateEulerXyz(Vec3 point, Vec3 eulerRadians) {
  const float cx = std::cos(eulerRadians.x);
  const float sx = std::sin(eulerRadians.x);
  const float cy = std::cos(eulerRadians.y);
  const float sy = std::sin(eulerRadians.y);
  const float cz = std::cos(eulerRadians.z);
  const float sz = std::sin(eulerRadians.z);

  const float y1 = point.y * cx - point.z * sx;
  const float z1 = point.y * sx + point.z * cx;
  const float x1 = point.x;

  const float x2 = x1 * cy + z1 * sy;
  const float z2 = -x1 * sy + z1 * cy;
  const float y2 = y1;

  const float x3 = x2 * cz - y2 * sz;
  const float y3 = x2 * sz + y2 * cz;
  const float z3 = z2;
  return Vec3{x3, y3, z3};
}

}  // namespace iggy3d
