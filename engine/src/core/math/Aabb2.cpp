#include "core/math/Aabb2.hpp"

namespace iggy {

bool Aabb2::contains(Vec2 point) const
{
	return point.x >= min.x && point.y >= min.y && point.x <= max.x && point.y <= max.y;
}

bool Aabb2::overlaps(Aabb2 other) const
{
	return min.x <= other.max.x && max.x >= other.min.x && min.y <= other.max.y && max.y >= other.min.y;
}

} // namespace iggy
