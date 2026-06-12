#include "servers/physics2d/CollisionOverlap2D.hpp"

#include <algorithm>

namespace iggy::physics2d {

namespace {

Aabb2 Normalize(Aabb2 bounds)
{
	return {
		{ std::min(bounds.min.x, bounds.max.x), std::min(bounds.min.y, bounds.max.y) },
		{ std::max(bounds.min.x, bounds.max.x), std::max(bounds.min.y, bounds.max.y) },
	};
}

} // namespace

bool CollisionOverlap2DResult::hasHits() const
{
	return !hits.empty();
}

CollisionOverlap2DResult CollisionOverlap2D::queryAabb(const CollisionWorld2D &world, Aabb2 queryBounds) const
{
	CollisionOverlap2DResult result;
	const Aabb2 normalizedQuery = Normalize(queryBounds);
	const std::vector<CollisionObject2D> &objects = world.objects();

	for (std::size_t index = 0; index < objects.size(); ++index) {
		const CollisionObject2D &object = objects[index];
		if (!isValid(object.shape))
			continue;

		const Aabb2 objectBounds = boundsOf(object.shape);
		if (!normalizedQuery.overlaps(objectBounds))
			continue;

		result.hits.push_back({ index, object, objectBounds });
	}

	return result;
}

} // namespace iggy::physics2d
