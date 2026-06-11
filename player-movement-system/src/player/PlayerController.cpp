#include "PlayerController.hpp"

namespace dev {

namespace {

void EmitControllerEvent(MovementEventSink *eventSink, MovementEventType type, PlayerId playerId, Point tile, DestinationActionType actionType = DestinationActionType::None)
{
	if (eventSink == nullptr)
		return;
	eventSink->emit({
		.type = type,
		.playerId = playerId,
		.tile = tile,
		.actionType = actionType,
	});
}

} // namespace

PlayerController::PlayerController(std::vector<Player> &players, const TileMap &map, const Collision &collision, const PathFinder &pathFinder, MovementEventSink *eventSink)
    : players_(players)
    , map_(map)
    , collision_(collision)
    , pathFinder_(pathFinder)
    , eventSink_(eventSink)
{
}

void PlayerController::walkTo(PlayerId playerId, Point destination)
{
	if (playerId >= players_.size() || !map_.isWalkable(destination)) {
		EmitControllerEvent(eventSink_, MovementEventType::CommandRejected, playerId, destination);
		return;
	}

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
	EmitControllerEvent(eventSink_, player.path.empty() ? MovementEventType::PathBlocked : MovementEventType::PathStarted, playerId, destination);
}

void PlayerController::moveThenAct(PlayerId playerId, Point destination, DestinationAction action)
{
	if (playerId >= players_.size())
		return;

	Player &player = players_[playerId];
	player.destinationAction = action;
	player.path = pathFinder_.findPath(player.position.future, destination, map_, collision_);
	player.moveState = player.path.empty() ? PlayerMoveState::Acting : PlayerMoveState::Pathing;
	EmitControllerEvent(eventSink_, player.path.empty() ? MovementEventType::DestinationActionReady : MovementEventType::PathStarted, playerId, destination, action.type);
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
	EmitControllerEvent(eventSink_, MovementEventType::DestinationActionReady, playerId, action.target.tile, action.type);
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
