#include "core/math/Vec3.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {
using iggy3d::cross;
using iggy3d::dot;
using iggy3d::length;
using iggy3d::nearlyEqual;
using iggy3d::normalized;
using iggy3d::normalizedOr;
using iggy3d::tryNormalize;
using iggy3d::Vec3;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(float a, float b, float tol = 1e-5F) { return std::fabs(a - b) <= tol; }

bool crossOfUnitAxes() {
  // Right-handed: x cross y = z, y cross z = x, z cross x = y; anticommutative.
  return expect(nearlyEqual(cross(Vec3{1, 0, 0}, Vec3{0, 1, 0}), Vec3{0, 0, 1}), "x^y=z") &&
         expect(nearlyEqual(cross(Vec3{0, 1, 0}, Vec3{0, 0, 1}), Vec3{1, 0, 0}), "y^z=x") &&
         expect(nearlyEqual(cross(Vec3{0, 0, 1}, Vec3{1, 0, 0}), Vec3{0, 1, 0}), "z^x=y") &&
         expect(nearlyEqual(cross(Vec3{0, 1, 0}, Vec3{1, 0, 0}), Vec3{0, 0, -1}),
                "y^x=-z (anticommutative)");
}

bool crossOfParallelIsZero() {
  return expect(nearlyEqual(cross(Vec3{2, 3, 4}, Vec3{2, 3, 4}), Vec3{0, 0, 0}), "v^v=0") &&
         expect(nearlyEqual(cross(Vec3{1, 2, 3}, Vec3{2, 4, 6}), Vec3{0, 0, 0}), "v^(2v)=0");
}

bool crossIsPerpendicular() {
  const Vec3 a{1, 2, 3};
  const Vec3 b{4, 5, 6};
  const Vec3 c = cross(a, b);
  return expect(near(dot(c, a), 0.0F), "cross perpendicular to a") &&
         expect(near(dot(c, b), 0.0F), "cross perpendicular to b");
}

bool lengthMatchesGeometry() {
  return expect(near(length(Vec3{3, 4, 0}), 5.0F), "3-4-5 length is 5") &&
         expect(near(length(Vec3{0, 0, 0}), 0.0F), "zero length is 0") &&
         expect(near(length(Vec3{0, 0, 2}), 2.0F), "axis length");
}

bool normalizedIsUnitAndPreservesDirection() {
  const Vec3 n = normalized(Vec3{0, 3, 4});
  return expect(near(length(n), 1.0F), "normalized is unit length") &&
         expect(nearlyEqual(n, Vec3{0, 0.6F, 0.8F}), "direction preserved (0,0.6,0.8)") &&
         expect(nearlyEqual(normalized(Vec3{5, 0, 0}), Vec3{1, 0, 0}),
                "already-axis normalizes to the axis");
}

bool degenerateNormalizeIsSafe() {
  const float inf = std::numeric_limits<float>::infinity();
  Vec3 out{9, 9, 9};
  return expect(nearlyEqual(normalized(Vec3{0, 0, 0}), Vec3{0, 0, 0}),
                "zero vector normalizes to zero") &&
         expect(nearlyEqual(normalizedOr(Vec3{0, 0, 0}, Vec3{1, 0, 0}), Vec3{1, 0, 0}),
                "normalizedOr uses the fallback when degenerate") &&
         expect(nearlyEqual(normalizedOr(Vec3{0, 5, 0}, Vec3{1, 0, 0}), Vec3{0, 1, 0}),
                "normalizedOr normalizes when it can") &&
         expect(!tryNormalize(Vec3{0, 0, 0}, out),
                "tryNormalize reports degenerate for zero") &&
         expect(nearlyEqual(out, Vec3{9, 9, 9}), "tryNormalize leaves out untouched on failure") &&
         expect(!tryNormalize(Vec3{inf, 0, 0}, out), "tryNormalize rejects non-finite");
}

bool tryNormalizeSucceedsForRealVectors() {
  Vec3 out{};
  const bool ok = tryNormalize(Vec3{0, 3, 4}, out);
  return expect(ok, "tryNormalize succeeds for a real vector") &&
         expect(near(length(out), 1.0F), "tryNormalize output is unit length");
}

}  // namespace

int main() {
  const bool ok = crossOfUnitAxes() && crossOfParallelIsZero() &&
                  crossIsPerpendicular() && lengthMatchesGeometry() &&
                  normalizedIsUnitAndPreservesDirection() &&
                  degenerateNormalizeIsSafe() && tryNormalizeSucceedsForRealVectors();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
