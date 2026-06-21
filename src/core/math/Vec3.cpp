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

float lengthSquared(Vec3 value) {
  return dot(value, value);
}

float distanceSquared(Vec3 lhs, Vec3 rhs) {
  return lengthSquared(lhs - rhs);
}

bool isFinite(Vec3 value) {
  return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

}  // namespace iggy3d
