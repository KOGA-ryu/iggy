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
    , paths_(map, collision, pathFinder, eventSink)
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

	paths_.walkTo(player, playerId, destination);
}

void PlayerController::moveThenAct(PlayerId playerId, Point destination, DestinationAction action)
{
	if (playerId >= players_.size())
		return;

	Player &player = players_[playerId];
	paths_.moveThenAct(player, playerId, destination, action);
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
