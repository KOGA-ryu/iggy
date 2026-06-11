#include "PlayerPathStepper.hpp"

namespace dev {

namespace {

void EmitMovementEvent(MovementEventSink *eventSink, MovementEventType type, Point tile, DestinationActionType actionType = DestinationActionType::None)
{
	if (eventSink == nullptr)
		return;
	eventSink->emit({
	    .type = type,
	    .tile = tile,
	    .actionType = actionType,
	});
}

} // namespace

PlayerPathStepper::PlayerPathStepper(const Collision &collision, MovementEventSink *eventSink)
    : collision_(collision)
    , eventSink_(eventSink)
{
}

PlayerPathStepResult PlayerPathStepper::step(Player &player) const
{
	const std::optional<Point> maybeNextTile = player.path.popNext();
	if (!maybeNextTile.has_value())
		return {};

	const Point nextTile = *maybeNextTile;
	if (collision_.blocksMovement(nextTile)) {
		player.moveState = PlayerMoveState::Blocked;
		player.path.clear();
		player.position.future = player.position.tile;
		EmitMovementEvent(eventSink_, MovementEventType::PathBlocked, nextTile);
		return { .consumedStep = true };
	}

	player.position.previous = player.position.tile;
	player.position.future = nextTile;
	player.position.tile = nextTile;
	player.position.precise = nextTile;
	EmitMovementEvent(eventSink_, MovementEventType::StepCommitted, nextTile);

	player.moveState = player.path.empty()
	    ? (player.destinationAction.type == DestinationActionType::None ? PlayerMoveState::Idle : PlayerMoveState::Acting)
	    : PlayerMoveState::Pathing;

	if (player.moveState != PlayerMoveState::Acting)
		return { .consumedStep = true };

	EmitMovementEvent(eventSink_, MovementEventType::DestinationActionReady, player.position.tile, player.destinationAction.type);
	return { .consumedStep = true, .actionReady = true };
}

} // namespace dev
