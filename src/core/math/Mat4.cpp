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

ProjectedPoint3 projectPoint(const Mat4& matrix, Vec3 point) {
  const float x = at(matrix, 0, 0) * point.x + at(matrix, 0, 1) * point.y +
                  at(matrix, 0, 2) * point.z + at(matrix, 0, 3);
  const float y = at(matrix, 1, 0) * point.x + at(matrix, 1, 1) * point.y +
                  at(matrix, 1, 2) * point.z + at(matrix, 1, 3);
  const float z = at(matrix, 2, 0) * point.x + at(matrix, 2, 1) * point.y +
                  at(matrix, 2, 2) * point.z + at(matrix, 2, 3);
  const float w = at(matrix, 3, 0) * point.x + at(matrix, 3, 1) * point.y +
                  at(matrix, 3, 2) * point.z + at(matrix, 3, 3);
  Vec3 ndc{x, y, z};
  if (std::isfinite(w) && w != 0.0F && w != 1.0F) {
    ndc = {x / w, y / w, z / w};
  }
  return {ndc, w, isFinite(ndc) && std::isfinite(w)};
}

Vec3 transformPoint(const Mat4& matrix, Vec3 point) {
  return projectPoint(matrix, point).ndc;
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
