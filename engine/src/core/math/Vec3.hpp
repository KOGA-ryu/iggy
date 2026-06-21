#pragma once

#include <cmath>

namespace iggy {

struct Vec3 {
	float x = 0.0F;
	float y = 0.0F;
	float z = 0.0F;

	[[nodiscard]] float lengthSquared() const;
	[[nodiscard]] float length() const;
	[[nodiscard]] Vec3 normalized() const;
};

[[nodiscard]] Vec3 operator+(Vec3 left, Vec3 right);
[[nodiscard]] Vec3 operator-(Vec3 left, Vec3 right);
[[nodiscard]] Vec3 operator*(Vec3 value, float scalar);
[[nodiscard]] Vec3 operator/(Vec3 value, float scalar);
[[nodiscard]] bool operator==(Vec3 left, Vec3 right);
[[nodiscard]] float Dot(Vec3 left, Vec3 right);

} // namespace iggy
