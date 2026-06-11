#include "PlayerPathPlanner.hpp"

namespace dev {

PlayerPathPlanner::PlayerPathPlanner(const TileMap &map, const Collision &collision, const PathFinder &pathFinder, MovementEventSink *eventSink)
    : map_(map)
    , collision_(collision)
    , pathFinder_(pathFinder)
    , eventSink_(eventSink)
{
}

void PlayerPathPlanner::walkTo(Player &player, PlayerId playerId, Point destination) const
{
	player.destinationAction = {};
	player.path = pathFinder_.findPath(player.position.future, destination, map_, collision_);
	player.moveState = player.path.empty() ? PlayerMoveState::Blocked : PlayerMoveState::Pathing;
	emit(player.path.empty() ? MovementEventType::PathBlocked : MovementEventType::PathStarted, playerId, destination);
}

void PlayerPathPlanner::moveThenAct(Player &player, PlayerId playerId, Point destination, DestinationAction action) const
{
	player.destinationAction = action;
	player.path = pathFinder_.findPath(player.position.future, destination, map_, collision_);
	player.moveState = player.path.empty() ? PlayerMoveState::Acting : PlayerMoveState::Pathing;
	emit(player.path.empty() ? MovementEventType::DestinationActionReady : MovementEventType::PathStarted, playerId, destination, action.type);
}

void PlayerPathPlanner::emit(MovementEventType type, PlayerId playerId, Point tile, DestinationActionType actionType) const
{
	if (eventSink_ == nullptr)
		return;
	eventSink_->emit({
	    .type = type,
	    .playerId = playerId,
	    .tile = tile,
	    .actionType = actionType,
	});
}

} // namespace dev
