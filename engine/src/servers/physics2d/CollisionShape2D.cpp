#include "servers/physics2d/CollisionShape2D.hpp"

namespace iggy::physics2d {

CollisionShape2D makeAabbShape(Aabb2 bounds)
{
	return { CollisionShape2DType::Aabb, bounds };
}

Aabb2 boundsOf(const CollisionShape2D &shape)
{
	return shape.bounds;
}

bool isValid(const CollisionShape2D &shape)
{
	if (shape.type != CollisionShape2DType::Aabb)
		return false;

	return shape.bounds.min.x <= shape.bounds.max.x && shape.bounds.min.y <= shape.bounds.max.y;
}

} // namespace iggy::physics2d
