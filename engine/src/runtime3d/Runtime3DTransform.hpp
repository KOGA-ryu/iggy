#pragma once

#include "core/math/Vec3.hpp"

namespace iggy::runtime3d {

struct Runtime3DTransform {
	iggy::Vec3 position;
	float yaw = 0.0F;
	float pitch = 0.0F;
	float roll = 0.0F;
	iggy::Vec3 scale { 1.0F, 1.0F, 1.0F };
};

} // namespace iggy::runtime3d
