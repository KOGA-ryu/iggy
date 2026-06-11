#include "PlayerMovement.hpp"

namespace dev {

PlayerMovement::PlayerMovement(const Collision &collision, ActionExecutor actionExecutor)
    : collision_(collision)
    , actionExecutor_(actionExecutor)
{
}

void PlayerMovement::update(std::vector<Player> &players, float deltaSeconds) const
{
	for (Player &player : players) {
		if (player.animationLock.active) {
			player.animationLock.elapsedSeconds += deltaSeconds;
			if (!player.animationLock.canCancel())
				continue;
			player.animationLock.active = false;
		}

		if (player.path.empty()) {
			player.moveState = player.destinationAction.type == DestinationActionType::None
			    ? PlayerMoveState::Idle
			    : PlayerMoveState::Acting;
			if (player.moveState == PlayerMoveState::Acting)
				actionExecutor_.update(player);
			continue;
		}

		const std::optional<Point> maybeNextTile = player.path.popNext();
		if (!maybeNextTile.has_value())
			continue;

		const Point nextTile = *maybeNextTile;
		if (collision_.blocksMovement(nextTile)) {
			player.moveState = PlayerMoveState::Blocked;
			player.path.clear();
			player.position.future = player.position.tile;
			continue;
		}

		player.position.previous = player.position.tile;
		player.position.future = nextTile;
		player.position.tile = nextTile;
		player.position.precise = nextTile;
		player.moveState = player.path.empty()
		    ? (player.destinationAction.type == DestinationActionType::None ? PlayerMoveState::Idle : PlayerMoveState::Acting)
		    : PlayerMoveState::Pathing;
		if (player.moveState == PlayerMoveState::Acting)
			actionExecutor_.update(player);
	}
}

} // namespace dev
