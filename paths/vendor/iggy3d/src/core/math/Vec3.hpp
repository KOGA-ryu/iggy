#pragma once

namespace iggy3d {

struct Vec3 {
  float x = 0.0F;
  float y = 0.0F;
  float z = 0.0F;
};

Vec3 vec3Zero();
Vec3 vec3UnitX();
Vec3 vec3UnitY();
Vec3 vec3UnitZ();
Vec3 operator+(Vec3 lhs, Vec3 rhs);
Vec3 operator-(Vec3 lhs, Vec3 rhs);
Vec3 operator*(Vec3 value, float scalar);
Vec3 operator*(float scalar, Vec3 value);
Vec3 operator/(Vec3 value, float scalar);
bool nearlyEqual(Vec3 lhs, Vec3 rhs, float epsilon = 0.0001F);
float dot(Vec3 lhs, Vec3 rhs);
Vec3 cross(Vec3 lhs, Vec3 rhs);
float lengthSquared(Vec3 value);
float length(Vec3 value);
float distanceSquared(Vec3 lhs, Vec3 rhs);
bool isFinite(Vec3 value);

// Unit-length copy. A vector too small (or non-finite) to normalize is degenerate: normalized()
// returns {0,0,0}, normalizedOr() returns `fallback`, tryNormalize() returns false and leaves `out`
// untouched. Callers that must not silently get a zero direction should use tryNormalize/normalizedOr.
Vec3 normalized(Vec3 value);
Vec3 normalizedOr(Vec3 value, Vec3 fallback);
bool tryNormalize(Vec3 value, Vec3& out);

}  // namespace iggy3d
