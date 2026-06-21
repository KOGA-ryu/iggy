#pragma once

#include <array>
#include <cstdint>

#include "core/math/Vec3.hpp"

namespace iggy3d {

struct Mat4 {
  std::array<float, 16> m;
};

Mat4 identityMat4();
Mat4 translationMat4(Vec3 translation);
Mat4 scaleMat4(Vec3 scale);
Mat4 rotationEulerRadiansMat4(Vec3 rotationEulerRadians);
Mat4 operator*(const Mat4& lhs, const Mat4& rhs);
Vec3 transformPoint(const Mat4& matrix, Vec3 point);
bool isFinite(const Mat4& matrix);
float at(const Mat4& matrix, std::uint32_t row, std::uint32_t column);

}  // namespace iggy3d
