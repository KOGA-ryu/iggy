#include "core/math/Vec2.hpp"

namespace iggy {

float Vec2::lengthSquared() const
{
	return Dot(*this, *this);
}

float Vec2::length() const
{
	return std::sqrt(lengthSquared());
}

Vec2 Vec2::normalized() const
{
	const float magnitude = length();
	if (magnitude == 0.0F)
		return {};
	return *this / magnitude;
}

Vec2 operator+(Vec2 left, Vec2 right)
{
	return { left.x + right.x, left.y + right.y };
}

Vec2 operator-(Vec2 left, Vec2 right)
{
	return { left.x - right.x, left.y - right.y };
}

Vec2 operator*(Vec2 value, float scalar)
{
	return { value.x * scalar, value.y * scalar };
}

Vec2 operator/(Vec2 value, float scalar)
{
	return { value.x / scalar, value.y / scalar };
}

bool operator==(Vec2 left, Vec2 right)
{
	return left.x == right.x && left.y == right.y;
}

float Dot(Vec2 left, Vec2 right)
{
	return left.x * right.x + left.y * right.y;
}

} // namespace iggy
