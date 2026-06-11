#include "PlayerActionRunner.hpp"

namespace dev {

PlayerActionRunner::PlayerActionRunner(ActionExecutor actionExecutor)
    : actionExecutor_(actionExecutor)
{
}

void PlayerActionRunner::run(Player &player, PlayerId playerId) const
{
	player.moveState = player.destinationAction.type == DestinationActionType::None
	    ? PlayerMoveState::Idle
	    : PlayerMoveState::Acting;

	if (player.moveState == PlayerMoveState::Acting)
		actionExecutor_.update(player, playerId);
}

} // namespace dev
