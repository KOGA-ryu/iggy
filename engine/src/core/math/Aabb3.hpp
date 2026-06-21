#pragma once

#include "core/math/Vec3.hpp"

namespace iggy {

struct Aabb3 {
	Vec3 min;
	Vec3 max;

	[[nodiscard]] bool contains(Vec3 point) const;
	[[nodiscard]] bool overlaps(Aabb3 other) const;
};

} // namespace iggy
