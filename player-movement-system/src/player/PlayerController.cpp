#include "PlayerController.hpp"

namespace dev {

PlayerController::PlayerController(std::vector<Player> &players, const TileMap &map, const Collision &collision, const PathFinder &pathFinder)
    : players_(players)
    , map_(map)
    , collision_(collision)
    , pathFinder_(pathFinder)
{
}

void PlayerController::walkTo(PlayerId playerId, Point destination)
{
	if (playerId >= players_.size() || !map_.isWalkable(destination))
		return;

	Player &player = players_[playerId];
	player.destinationAction = {};
	if (player.movementModifiers.standGround) {
		player.path.clear();
		player.position.future = player.position.tile;
		player.moveState = PlayerMoveState::Idle;
		return;
	}

	player.path = pathFinder_.findPath(player.position.future, destination, map_, collision_);
	player.moveState = player.path.empty() ? PlayerMoveState::Blocked : PlayerMoveState::Pathing;
}

void PlayerController::moveThenAct(PlayerId playerId, Point destination, DestinationAction action)
{
	if (playerId >= players_.size())
		return;

	Player &player = players_[playerId];
	player.destinationAction = action;
	player.path = pathFinder_.findPath(player.position.future, destination, map_, collision_);
	player.moveState = player.path.empty() ? PlayerMoveState::Acting : PlayerMoveState::Pathing;
}

void PlayerController::standAndAct(PlayerId playerId, DestinationAction action)
{
	if (playerId >= players_.size())
		return;

	Player &player = players_[playerId];
	player.path.clear();
	player.position.future = player.position.tile;
	player.destinationAction = action;
	player.moveState = PlayerMoveState::Acting;
}

void PlayerController::stop(PlayerId playerId)
{
	if (playerId >= players_.size())
		return;

	Player &player = players_[playerId];
	player.path.clear();
	player.destinationAction = {};
	player.position.future = player.position.tile;
	player.moveState = PlayerMoveState::Idle;
}

} // namespace dev
