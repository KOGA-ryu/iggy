#include "core/math/Vec3.hpp"

namespace iggy {

float Vec3::lengthSquared() const
{
	return Dot(*this, *this);
}

float Vec3::length() const
{
	return std::sqrt(lengthSquared());
}

Vec3 Vec3::normalized() const
{
	const float magnitude = length();
	if (magnitude == 0.0F)
		return {};
	return *this / magnitude;
}

Vec3 operator+(Vec3 left, Vec3 right)
{
	return { left.x + right.x, left.y + right.y, left.z + right.z };
}

Vec3 operator-(Vec3 left, Vec3 right)
{
	return { left.x - right.x, left.y - right.y, left.z - right.z };
}

Vec3 operator*(Vec3 value, float scalar)
{
	return { value.x * scalar, value.y * scalar, value.z * scalar };
}

Vec3 operator/(Vec3 value, float scalar)
{
	return { value.x / scalar, value.y / scalar, value.z / scalar };
}

bool operator==(Vec3 left, Vec3 right)
{
	return left.x == right.x && left.y == right.y && left.z == right.z;
}

float Dot(Vec3 left, Vec3 right)
{
	return left.x * right.x + left.y * right.y + left.z * right.z;
}

} // namespace iggy
