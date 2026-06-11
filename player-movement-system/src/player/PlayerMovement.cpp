#include "PlayerMovement.hpp"

namespace dev {

PlayerMovement::PlayerMovement(const Collision &collision, ActionExecutor actionExecutor, MovementEventSink *eventSink)
    : animationLocks_(eventSink)
    , pathStepper_(collision, eventSink)
    , actionExecutor_(actionExecutor)
{
}

void PlayerMovement::update(std::vector<Player> &players, float deltaSeconds) const
{
	for (std::size_t index = 0; index < players.size(); ++index) {
		Player &player = players[index];
		const PlayerId playerId = static_cast<PlayerId>(index);
		if (!animationLocks_.advance(player, deltaSeconds))
			continue;

		if (player.path.empty()) {
			player.moveState = player.destinationAction.type == DestinationActionType::None
			    ? PlayerMoveState::Idle
			    : PlayerMoveState::Acting;
			if (player.moveState == PlayerMoveState::Acting)
				actionExecutor_.update(player, playerId);
			continue;
		}

		const PlayerPathStepResult step = pathStepper_.step(player);
		if (step.actionReady)
			actionExecutor_.update(player, playerId);
	}
}

} // namespace dev
