#pragma once

#include <array>
#include <cstdint>

#include "core/math/Vec3.hpp"

namespace iggy3d {

struct Mat4 {
  std::array<float, 16> m;
};

struct ProjectedPoint3 {
  Vec3 ndc{};
  float w = 1.0F;
  bool finite = false;
};

Mat4 identityMat4();
Mat4 operator*(const Mat4& lhs, const Mat4& rhs);
ProjectedPoint3 projectPoint(const Mat4& matrix, Vec3 point);
Vec3 transformPoint(const Mat4& matrix, Vec3 point);
bool isFinite(const Mat4& matrix);
float at(const Mat4& matrix, std::uint32_t row, std::uint32_t column);

}  // namespace iggy3d
