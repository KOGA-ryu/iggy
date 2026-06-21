#pragma once

#include "core/math/Vec3.hpp"

namespace iggy {

struct Transform3 {
	Vec3 position;
	Vec3 yawPitchRoll;
	Vec3 scale { 1.0F, 1.0F, 1.0F };
};

[[nodiscard]] bool operator==(Transform3 left, Transform3 right);

} // namespace iggy
