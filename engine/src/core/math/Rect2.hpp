#pragma once

#include "core/math/Vec2.hpp"

namespace iggy {

struct Aabb2;

struct Rect2 {
	Vec2 position;
	Vec2 size;

	[[nodiscard]] Vec2 min() const;
	[[nodiscard]] Vec2 max() const;
	[[nodiscard]] Aabb2 toAabb() const;
	[[nodiscard]] bool contains(Vec2 point) const;
	[[nodiscard]] bool overlaps(Rect2 other) const;
};

} // namespace iggy
