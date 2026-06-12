#pragma once

#include <cmath>

namespace iggy {

struct Vec2 {
	float x = 0.0F;
	float y = 0.0F;

	[[nodiscard]] float lengthSquared() const;
	[[nodiscard]] float length() const;
	[[nodiscard]] Vec2 normalized() const;
};

[[nodiscard]] Vec2 operator+(Vec2 left, Vec2 right);
[[nodiscard]] Vec2 operator-(Vec2 left, Vec2 right);
[[nodiscard]] Vec2 operator*(Vec2 value, float scalar);
[[nodiscard]] Vec2 operator/(Vec2 value, float scalar);
[[nodiscard]] bool operator==(Vec2 left, Vec2 right);
[[nodiscard]] float Dot(Vec2 left, Vec2 right);

} // namespace iggy
