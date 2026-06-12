#pragma once

#include "core/math/Vec2.hpp"

namespace iggy {

struct Ray2 {
	Vec2 origin;
	Vec2 direction;

	[[nodiscard]] Vec2 pointAtDistance(float distance) const;
};

} // namespace iggy
