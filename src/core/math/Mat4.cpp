#include "core/math/Mat4.hpp"

#include <cmath>

namespace iggy3d {

namespace {
std::size_t indexOf(std::uint32_t row, std::uint32_t column) {
  return static_cast<std::size_t>(row) * 4U + static_cast<std::size_t>(column);
}
}  // namespace

Mat4 identityMat4() {
  return {{{1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F,
            0.0F, 0.0F, 0.0F, 0.0F, 1.0F}}};
}

Mat4 translationMat4(Vec3 translation) {
  Mat4 result = identityMat4();
  result.m[indexOf(0, 3)] = translation.x;
  result.m[indexOf(1, 3)] = translation.y;
  result.m[indexOf(2, 3)] = translation.z;
  return result;
}

Mat4 scaleMat4(Vec3 scale) {
  Mat4 result = identityMat4();
  result.m[indexOf(0, 0)] = scale.x;
  result.m[indexOf(1, 1)] = scale.y;
  result.m[indexOf(2, 2)] = scale.z;
  return result;
}

Mat4 rotationEulerRadiansMat4(Vec3 rotationEulerRadians) {
  const float pitch = rotationEulerRadians.x;
  const float yaw = rotationEulerRadians.y;
  const float roll = rotationEulerRadians.z;
  const float cp = std::cos(pitch);
  const float sp = std::sin(pitch);
  const float cy = std::cos(yaw);
  const float sy = std::sin(yaw);
  const float cr = std::cos(roll);
  const float sr = std::sin(roll);
  Mat4 rollMatrix = {{{cr, -sr, 0.0F, 0.0F, sr, cr, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F,
                       0.0F, 0.0F, 0.0F, 0.0F, 1.0F}}};
  Mat4 pitchMatrix = {{{1.0F, 0.0F, 0.0F, 0.0F, 0.0F, cp, -sp, 0.0F, 0.0F, sp,
                        cp, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F}}};
  Mat4 yawMatrix = {{{cy, 0.0F, sy, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, -sy, 0.0F, cy,
                      0.0F, 0.0F, 0.0F, 0.0F, 1.0F}}};
  return yawMatrix * (pitchMatrix * rollMatrix);
}

Mat4 operator*(const Mat4& lhs, const Mat4& rhs) {
  Mat4 out{{{}}};
  for (std::uint32_t row = 0; row < 4U; ++row) {
    for (std::uint32_t column = 0; column < 4U; ++column) {
      float sum = 0.0F;
      for (std::uint32_t k = 0; k < 4U; ++k) {
        sum += at(lhs, row, k) * at(rhs, k, column);
      }
      out.m[indexOf(row, column)] = sum;
    }
  }
  return out;
}

Vec3 transformPoint(const Mat4& matrix, Vec3 point) {
  const float x = at(matrix, 0, 0) * point.x + at(matrix, 0, 1) * point.y +
                  at(matrix, 0, 2) * point.z + at(matrix, 0, 3);
  const float y = at(matrix, 1, 0) * point.x + at(matrix, 1, 1) * point.y +
                  at(matrix, 1, 2) * point.z + at(matrix, 1, 3);
  const float z = at(matrix, 2, 0) * point.x + at(matrix, 2, 1) * point.y +
                  at(matrix, 2, 2) * point.z + at(matrix, 2, 3);
  const float w = at(matrix, 3, 0) * point.x + at(matrix, 3, 1) * point.y +
                  at(matrix, 3, 2) * point.z + at(matrix, 3, 3);
  if (std::isfinite(w) && w != 0.0F && w != 1.0F) {
    return {x / w, y / w, z / w};
  }
  return {x, y, z};
}

bool isFinite(const Mat4& matrix) {
  for (float value : matrix.m) {
    if (!std::isfinite(value)) {
      return false;
    }
  }
  return true;
}

float at(const Mat4& matrix, std::uint32_t row, std::uint32_t column) {
  return matrix.m[indexOf(row, column)];
}

}  // namespace iggy3d
