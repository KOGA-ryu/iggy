#include "core/math/Rect2.hpp"

#include "core/math/Aabb2.hpp"

namespace iggy {

Vec2 Rect2::min() const
{
	return position;
}

Vec2 Rect2::max() const
{
	return position + size;
}

Aabb2 Rect2::toAabb() const
{
	return Aabb2 { min(), max() };
}

bool Rect2::contains(Vec2 point) const
{
	return toAabb().contains(point);
}

bool Rect2::overlaps(Rect2 other) const
{
	return toAabb().overlaps(other.toAabb());
}

} // namespace iggy
