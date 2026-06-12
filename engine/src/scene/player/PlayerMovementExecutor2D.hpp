#pragma once

#include "core/math/Vec2.hpp"
#include "scene/player/PlayerAgentState.hpp"
#include "scene/player/PlayerCommandPlan2D.hpp"
#include "servers/physics2d/CharacterMove2D.hpp"
#include "servers/physics2d/CollisionWorld2D.hpp"

namespace iggy {

struct PlayerMovementExecutor2DConfig {
	Vec2 bodySize { 1.0F, 1.0F };
	Vec2 bodyAnchor { 0.5F, 0.5F };
	float maxStep = 1.0F;
};

enum class PlayerMovementExecutionStatus {
	NoMovement,
	Moved,
	Blocked,
	RejectedPlan,
	UnsupportedPlan,
};

struct PlayerMovementExecutionResult {
	PlayerMovementExecutionStatus status = PlayerMovementExecutionStatus::NoMovement;
	PlayerAgentState state;
	Vec2 requestedDelta;
	physics2d::CharacterMove2DResult movement;
	bool reachedTarget = false;
};

class PlayerMovementExecutor2D {
public:
	[[nodiscard]] PlayerMovementExecutionResult execute(
		const PlayerAgentState &player,
		const PlayerCommandPlan2D &plan,
		const physics2d::CollisionWorld2D &world,
		const PlayerMovementExecutor2DConfig &config) const;
};

} // namespace iggy
