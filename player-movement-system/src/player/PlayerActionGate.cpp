#include "PlayerActionGate.hpp"

namespace dev {

PlayerActionGate::PlayerActionGate(const InputFocus &focus, const PlayerActionContext &context)
    : focus_(focus)
    , context_(context)
{
}

bool PlayerActionGate::canMove(const Player &player) const
{
	return movementBlockReason(player) == PlayerActionBlockReason::None;
}

PlayerActionBlockReason PlayerActionGate::movementBlockReason(const Player &player) const
{
	if (!focus_.gameplayOwnsMovement())
		return PlayerActionBlockReason::Focus;
	if (context_.paused)
		return PlayerActionBlockReason::Paused;
	if (context_.animationLocked)
		return PlayerActionBlockReason::AnimationLocked;
	if (!player.animationLock.canCancel())
		return PlayerActionBlockReason::AnimationCommitment;
	if (player.moveState == PlayerMoveState::Stunned)
		return PlayerActionBlockReason::Stunned;
	return PlayerActionBlockReason::None;
}

} // namespace dev
