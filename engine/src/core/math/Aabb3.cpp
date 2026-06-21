#include "core/math/Aabb3.hpp"

namespace iggy {

bool Aabb3::contains(Vec3 point) const
{
	return point.x >= min.x && point.y >= min.y && point.z >= min.z &&
		point.x <= max.x && point.y <= max.y && point.z <= max.z;
}

bool Aabb3::overlaps(Aabb3 other) const
{
	return min.x <= other.max.x && max.x >= other.min.x &&
		min.y <= other.max.y && max.y >= other.min.y &&
		min.z <= other.max.z && max.z >= other.min.z;
}

} // namespace iggy
