#include "servers/physics2d/CollisionMotionQuery2D.hpp"

#include <algorithm>
#include <limits>
#include <vector>

namespace iggy::physics2d {

namespace {

struct AxisSweep {
	bool canHit = false;
	float entry = 0.0F;
	float exit = 0.0F;
};

Aabb2 Normalize(Aabb2 bounds)
{
	return {
		{ std::min(bounds.min.x, bounds.max.x), std::min(bounds.min.y, bounds.max.y) },
		{ std::max(bounds.min.x, bounds.max.x), std::max(bounds.min.y, bounds.max.y) },
	};
}

AxisSweep SweepAxis(float movingMin, float movingMax, float objectMin, float objectMax, float delta)
{
	if (delta == 0.0F) {
		if (movingMin <= objectMax && movingMax >= objectMin)
			return { true, -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity() };
		return {};
	}

	if (delta > 0.0F)
		return { true, (objectMin - movingMax) / delta, (objectMax - movingMin) / delta };

	return { true, (objectMax - movingMin) / delta, (objectMin - movingMax) / delta };
}

CollisionMotionQuery2DResult BlockedResult(
	float travel,
	Vec2 delta,
	std::size_t objectIndex,
	const CollisionObject2D &object,
	Aabb2 objectBounds)
{
	CollisionMotionQuery2DResult result;
	result.blocked = true;
	result.safeTravel = travel;
	result.allowedDelta = delta * travel;
	result.hit.hit = true;
	result.hit.objectIndex = objectIndex;
	result.hit.object = object;
	result.hit.objectBounds = objectBounds;
	result.hit.travel = travel;
	return result;
}

} // namespace

CollisionMotionQuery2DResult CollisionMotionQuery2D::sweepAabb(
	const CollisionWorld2D &world,
	Aabb2 movingBounds,
	Vec2 delta) const
{
	CollisionMotionQuery2DResult result;
	result.allowedDelta = delta;

	const Aabb2 normalizedMoving = Normalize(movingBounds);
	const std::vector<CollisionObject2D> &objects = world.objects();

	bool hasHit = false;
	float bestTravel = 1.0F;
	CollisionMotionHit2D bestHit;

	for (std::size_t index = 0; index < objects.size(); ++index) {
		const CollisionObject2D &object = objects[index];
		if (!isValid(object.shape))
			continue;

		const Aabb2 objectBounds = boundsOf(object.shape);
		if (normalizedMoving.overlaps(objectBounds)) {
			return BlockedResult(0.0F, delta, index, object, objectBounds);
		}

		const AxisSweep x = SweepAxis(normalizedMoving.min.x, normalizedMoving.max.x, objectBounds.min.x, objectBounds.max.x, delta.x);
		const AxisSweep y = SweepAxis(normalizedMoving.min.y, normalizedMoving.max.y, objectBounds.min.y, objectBounds.max.y, delta.y);
		if (!x.canHit || !y.canHit)
			continue;

		const float entry = std::max(x.entry, y.entry);
		const float exit = std::min(x.exit, y.exit);
		if (entry > exit || entry < 0.0F || entry > 1.0F)
			continue;

		if (hasHit && entry >= bestTravel)
			continue;

		hasHit = true;
		bestTravel = entry;
		bestHit.hit = true;
		bestHit.objectIndex = index;
		bestHit.object = object;
		bestHit.objectBounds = objectBounds;
		bestHit.travel = entry;
	}

	if (!hasHit)
		return result;

	result.blocked = true;
	result.safeTravel = bestTravel;
	result.allowedDelta = delta * bestTravel;
	result.hit = bestHit;
	return result;
}

} // namespace iggy::physics2d
