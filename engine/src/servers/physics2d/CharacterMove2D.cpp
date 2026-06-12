#include "servers/physics2d/CharacterMove2D.hpp"

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

Aabb2 Translate(Aabb2 bounds, Vec2 delta)
{
	return { bounds.min + delta, bounds.max + delta };
}

} // namespace

CharacterMove2DResult CharacterMove2D::move(
	const CollisionWorld2D &world,
	Aabb2 bounds,
	Vec2 requestedDelta) const
{
	CharacterMove2DResult result;
	result.startBounds = Normalize(bounds);
	result.requestedDelta = requestedDelta;
	result.motion = CollisionMotionQuery2D {}.sweepAabb(world, result.startBounds, requestedDelta);
	result.allowedDelta = result.motion.allowedDelta;
	result.finalBounds = Translate(result.startBounds, result.allowedDelta);

	if (result.motion.blocked) {
		result.status = CharacterMove2DStatus::Blocked;
	} else if (requestedDelta == Vec2 {}) {
		result.status = CharacterMove2DStatus::NoMovement;
	} else {
		result.status = CharacterMove2DStatus::Moved;
	}

	return result;
}

} // namespace iggy::physics2d
