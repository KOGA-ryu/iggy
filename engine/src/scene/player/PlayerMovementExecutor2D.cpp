#include "scene/player/PlayerMovementExecutor2D.hpp"

#include <algorithm>
#include <cmath>

namespace iggy {

namespace {

Aabb2 BodyBounds(const PlayerAgentState &player, const PlayerMovementExecutor2DConfig &config)
{
	const Vec2 effectiveSize { std::fabs(config.bodySize.x), std::fabs(config.bodySize.y) };
	const Vec2 min {
		player.position.x - effectiveSize.x * config.bodyAnchor.x,
		player.position.y - effectiveSize.y * config.bodyAnchor.y,
	};
	return { min, min + effectiveSize };
}

PlayerFacing2D FacingFromDelta(Vec2 delta, PlayerFacing2D fallback)
{
	if (delta == Vec2 {})
		return fallback;

	if (std::fabs(delta.x) >= std::fabs(delta.y))
		return delta.x >= 0.0F ? PlayerFacing2D::East : PlayerFacing2D::West;

	return delta.y >= 0.0F ? PlayerFacing2D::South : PlayerFacing2D::North;
}

void SetIdle(PlayerMovementExecutionResult &result)
{
	result.state.movementStatus = PlayerMovementStatus::Idle;
}

} // namespace

PlayerMovementExecutionResult PlayerMovementExecutor2D::execute(
	const PlayerAgentState &player,
	const PlayerCommandPlan2D &plan,
	const physics2d::CollisionWorld2D &world,
	const PlayerMovementExecutor2DConfig &config) const
{
	PlayerMovementExecutionResult result;
	result.state = player;

	switch (plan.type) {
	case PlayerCommandPlan2DType::Rejected:
		result.status = PlayerMovementExecutionStatus::RejectedPlan;
		SetIdle(result);
		return result;
	case PlayerCommandPlan2DType::Interact:
		result.status = PlayerMovementExecutionStatus::UnsupportedPlan;
		SetIdle(result);
		return result;
	case PlayerCommandPlan2DType::None:
	case PlayerCommandPlan2DType::Wait:
		result.status = PlayerMovementExecutionStatus::NoMovement;
		SetIdle(result);
		return result;
	case PlayerCommandPlan2DType::MoveToPoint:
		break;
	}

	const Vec2 deltaToTarget = plan.targetPoint - player.position;
	if (deltaToTarget == Vec2 {}) {
		result.status = PlayerMovementExecutionStatus::NoMovement;
		result.reachedTarget = true;
		SetIdle(result);
		return result;
	}

	if (config.maxStep <= 0.0F) {
		result.status = PlayerMovementExecutionStatus::NoMovement;
		SetIdle(result);
		return result;
	}

	result.requestedDelta = deltaToTarget.normalized() * std::min(deltaToTarget.length(), config.maxStep);
	result.state.facing = FacingFromDelta(result.requestedDelta, player.facing);
	result.movement = physics2d::CharacterMove2D {}.move(world, BodyBounds(player, config), result.requestedDelta);
	result.state.position = player.position + result.movement.allowedDelta;
	result.reachedTarget = result.state.position == plan.targetPoint;

	if (result.movement.status == physics2d::CharacterMove2DStatus::Blocked) {
		result.status = PlayerMovementExecutionStatus::Blocked;
	} else if (result.movement.allowedDelta == Vec2 {}) {
		result.status = PlayerMovementExecutionStatus::NoMovement;
	} else {
		result.status = PlayerMovementExecutionStatus::Moved;
	}

	if (result.movement.allowedDelta == Vec2 {} || result.reachedTarget)
		result.state.movementStatus = PlayerMovementStatus::Idle;
	else
		result.state.movementStatus = PlayerMovementStatus::Moving;

	return result;
}

} // namespace iggy
