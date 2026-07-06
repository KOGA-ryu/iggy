#include "core/math/Vec3.hpp"

#include <cmath>

namespace iggy3d {

Vec3 vec3Zero() {
  return {};
}

Vec3 vec3UnitX() {
  return {1.0F, 0.0F, 0.0F};
}

Vec3 vec3UnitY() {
  return {0.0F, 1.0F, 0.0F};
}

Vec3 vec3UnitZ() {
  return {0.0F, 0.0F, 1.0F};
}

Vec3 operator+(Vec3 lhs, Vec3 rhs) {
  return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

Vec3 operator-(Vec3 lhs, Vec3 rhs) {
  return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

Vec3 operator*(Vec3 value, float scalar) {
  return {value.x * scalar, value.y * scalar, value.z * scalar};
}

Vec3 operator*(float scalar, Vec3 value) {
  return value * scalar;
}

Vec3 operator/(Vec3 value, float scalar) {
  return {value.x / scalar, value.y / scalar, value.z / scalar};
}

bool nearlyEqual(Vec3 lhs, Vec3 rhs, float epsilon) {
  return std::fabs(lhs.x - rhs.x) <= epsilon && std::fabs(lhs.y - rhs.y) <= epsilon &&
         std::fabs(lhs.z - rhs.z) <= epsilon;
}

float dot(Vec3 lhs, Vec3 rhs) {
  return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

Vec3 cross(Vec3 lhs, Vec3 rhs) {
  return {lhs.y * rhs.z - lhs.z * rhs.y, lhs.z * rhs.x - lhs.x * rhs.z,
          lhs.x * rhs.y - lhs.y * rhs.x};
}

float lengthSquared(Vec3 value) {
  return dot(value, value);
}

float length(Vec3 value) {
  return std::sqrt(lengthSquared(value));
}

float distanceSquared(Vec3 lhs, Vec3 rhs) {
  return lengthSquared(lhs - rhs);
}

bool isFinite(Vec3 value) {
  return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool tryNormalize(Vec3 value, Vec3& out) {
  const float lengthSq = lengthSquared(value);
  if (!(lengthSq > 1e-20F) || !std::isfinite(lengthSq)) {
    return false;
  }
  out = value / std::sqrt(lengthSq);
  return true;
}

Vec3 normalizedOr(Vec3 value, Vec3 fallback) {
  Vec3 out;
  return tryNormalize(value, out) ? out : fallback;
}

Vec3 normalized(Vec3 value) {
  return normalizedOr(value, Vec3{0.0F, 0.0F, 0.0F});
}

}  // namespace iggy3d
