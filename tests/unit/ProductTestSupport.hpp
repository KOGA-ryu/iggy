#pragma once

#include "core/math/Mat4.hpp"

#include <cmath>
#include <cstddef>
#include <iostream>
#include <string_view>

namespace iggy3d::test {

inline bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

inline bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

inline bool near(float lhs, float rhs, float epsilon = 0.0001F) {
  return std::fabs(lhs - rhs) <= epsilon;
}

inline bool nearlyEqual(float lhs, float rhs, float epsilon = 0.0001F) {
  return near(lhs, rhs, epsilon);
}

inline bool nearlyEqual(const Mat4& lhs,
                        const Mat4& rhs,
                        float epsilon = 0.0001F) {
  for (std::size_t index = 0; index < lhs.m.size(); ++index) {
    if (!nearlyEqual(lhs.m[index], rhs.m[index], epsilon)) {
      return false;
    }
  }
  return true;
}

}  // namespace iggy3d::test
