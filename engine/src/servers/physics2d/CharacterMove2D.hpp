#pragma once

#include "core/math/Aabb2.hpp"
#include "core/math/Vec2.hpp"
#include "servers/physics2d/CollisionMotionQuery2D.hpp"
#include "servers/physics2d/CollisionWorld2D.hpp"

namespace iggy::physics2d {

enum class CharacterMove2DStatus {
	NoMovement,
	Moved,
	Blocked,
};

struct CharacterMove2DResult {
	CharacterMove2DStatus status = CharacterMove2DStatus::NoMovement;
	Aabb2 startBounds;
	Aabb2 finalBounds;
	Vec2 requestedDelta;
	Vec2 allowedDelta;
	CollisionMotionQuery2DResult motion;
};

class CharacterMove2D {
public:
	[[nodiscard]] CharacterMove2DResult move(
		const CollisionWorld2D &world,
		Aabb2 bounds,
		Vec2 requestedDelta) const;
};

} // namespace iggy::physics2d
