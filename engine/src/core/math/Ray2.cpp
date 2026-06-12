#include "core/math/Ray2.hpp"

namespace iggy {

Vec2 Ray2::pointAtDistance(float distance) const
{
	return origin + direction * distance;
}

} // namespace iggy
