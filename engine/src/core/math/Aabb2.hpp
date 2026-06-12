#pragma once

#include "core/math/Vec2.hpp"

namespace iggy {

struct Aabb2 {
	Vec2 min;
	Vec2 max;

	[[nodiscard]] bool contains(Vec2 point) const;
	[[nodiscard]] bool overlaps(Aabb2 other) const;
};

} // namespace iggy
