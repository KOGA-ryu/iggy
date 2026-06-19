#pragma once

#include <array>
#include <cmath>

namespace iggy::native_play {

struct Vec3 {
	float x = 0.0F;
	float y = 0.0F;
	float z = 0.0F;
};

struct Mat4 {
	std::array<float, 16> values {};
};

[[nodiscard]] inline Vec3 operator-(Vec3 left, Vec3 right)
{
	return { left.x - right.x, left.y - right.y, left.z - right.z };
}

[[nodiscard]] inline float Dot(Vec3 left, Vec3 right)
{
	return left.x * right.x + left.y * right.y + left.z * right.z;
}

[[nodiscard]] inline Vec3 Cross(Vec3 left, Vec3 right)
{
	return {
		left.y * right.z - left.z * right.y,
		left.z * right.x - left.x * right.z,
		left.x * right.y - left.y * right.x,
	};
}

[[nodiscard]] inline Vec3 Normalized(Vec3 value)
{
	const float length = std::sqrt(Dot(value, value));
	if (length == 0.0F)
		return {};
	return { value.x / length, value.y / length, value.z / length };
}

[[nodiscard]] inline Mat4 Identity()
{
	Mat4 matrix;
	matrix.values[0] = 1.0F;
	matrix.values[5] = 1.0F;
	matrix.values[10] = 1.0F;
	matrix.values[15] = 1.0F;
	return matrix;
}

[[nodiscard]] inline Mat4 Multiply(const Mat4 &left, const Mat4 &right)
{
	Mat4 result;
	for (int column = 0; column < 4; ++column) {
		for (int row = 0; row < 4; ++row) {
			float value = 0.0F;
			for (int k = 0; k < 4; ++k)
				value += left.values[k * 4 + row] * right.values[column * 4 + k];
			result.values[column * 4 + row] = value;
		}
	}
	return result;
}

[[nodiscard]] inline Mat4 RotationY(float radians)
{
	Mat4 matrix = Identity();
	const float c = std::cos(radians);
	const float s = std::sin(radians);
	matrix.values[0] = c;
	matrix.values[2] = -s;
	matrix.values[8] = s;
	matrix.values[10] = c;
	return matrix;
}

[[nodiscard]] inline Mat4 RotationX(float radians)
{
	Mat4 matrix = Identity();
	const float c = std::cos(radians);
	const float s = std::sin(radians);
	matrix.values[5] = c;
	matrix.values[6] = s;
	matrix.values[9] = -s;
	matrix.values[10] = c;
	return matrix;
}

[[nodiscard]] inline Mat4 Translation(Vec3 offset)
{
	Mat4 matrix = Identity();
	matrix.values[12] = offset.x;
	matrix.values[13] = offset.y;
	matrix.values[14] = offset.z;
	return matrix;
}

[[nodiscard]] inline Mat4 Scale(Vec3 value)
{
	Mat4 matrix = Identity();
	matrix.values[0] = value.x;
	matrix.values[5] = value.y;
	matrix.values[10] = value.z;
	return matrix;
}

[[nodiscard]] inline Mat4 Perspective(
	float fovRadians,
	float aspect,
	float nearPlane,
	float farPlane)
{
	const float f = 1.0F / std::tan(fovRadians * 0.5F);
	Mat4 matrix;
	matrix.values[0] = f / aspect;
	matrix.values[5] = -f;
	matrix.values[10] = farPlane / (nearPlane - farPlane);
	matrix.values[11] = -1.0F;
	matrix.values[14] = (farPlane * nearPlane) / (nearPlane - farPlane);
	return matrix;
}

[[nodiscard]] inline Mat4 LookAt(Vec3 eye, Vec3 center, Vec3 up)
{
	const Vec3 forward = Normalized(center - eye);
	const Vec3 side = Normalized(Cross(forward, up));
	const Vec3 cameraUp = Cross(side, forward);

	Mat4 matrix = Identity();
	matrix.values[0] = side.x;
	matrix.values[4] = side.y;
	matrix.values[8] = side.z;
	matrix.values[1] = cameraUp.x;
	matrix.values[5] = cameraUp.y;
	matrix.values[9] = cameraUp.z;
	matrix.values[2] = -forward.x;
	matrix.values[6] = -forward.y;
	matrix.values[10] = -forward.z;
	matrix.values[12] = -Dot(side, eye);
	matrix.values[13] = -Dot(cameraUp, eye);
	matrix.values[14] = Dot(forward, eye);
	return matrix;
}

} // namespace iggy::native_play
