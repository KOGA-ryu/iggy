#include "core/math/Ray3.hpp"

namespace iggy {

Vec3 Ray3::pointAtDistance(float distance) const
{
	return origin + direction * distance;
}

} // namespace iggy
