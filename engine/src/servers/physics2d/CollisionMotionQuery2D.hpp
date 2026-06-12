#pragma once

#include <cstddef>

#include "core/math/Aabb2.hpp"
#include "core/math/Vec2.hpp"
#include "servers/physics2d/CollisionWorld2D.hpp"

namespace iggy::physics2d {

struct CollisionMotionHit2D {
	bool hit = false;
	std::size_t objectIndex = 0;
	CollisionObject2D object;
	Aabb2 objectBounds;
	float travel = 1.0F;
};

struct CollisionMotionQuery2DResult {
	bool blocked = false;
	float safeTravel = 1.0F;
	Vec2 allowedDelta;
	CollisionMotionHit2D hit;
};

class CollisionMotionQuery2D {
public:
	[[nodiscard]] CollisionMotionQuery2DResult sweepAabb(
		const CollisionWorld2D &world,
		Aabb2 movingBounds,
		Vec2 delta) const;
};

} // namespace iggy::physics2d
