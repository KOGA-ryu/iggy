#include "PlayerActionGate.hpp"

namespace dev {

PlayerActionGate::PlayerActionGate(const InputFocus &focus, const PlayerActionContext &context)
    : focus_(focus)
    , context_(context)
{
}

bool PlayerActionGate::canMove(const Player &player) const
{
	if (!focus_.gameplayOwnsMovement())
		return false;
	if (context_.paused || context_.animationLocked)
		return false;
	if (!player.animationLock.canCancel())
		return false;
	if (player.moveState == PlayerMoveState::Stunned)
		return false;
	return true;
}

} // namespace dev
