#pragma once

#include "core/math/Vec3.hpp"

namespace iggy {

struct Ray3 {
	Vec3 origin;
	Vec3 direction;

	[[nodiscard]] Vec3 pointAtDistance(float distance) const;
};

} // namespace iggy
