#include "PlayerMovement.hpp"

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

PlayerMovement::PlayerMovement(const Collision &collision, ActionExecutor actionExecutor, MovementEventSink *eventSink)
    : collision_(collision)
    , actionExecutor_(actionExecutor)
    , eventSink_(eventSink)
{
}

void PlayerMovement::update(std::vector<Player> &players, float deltaSeconds) const
{
	for (std::size_t index = 0; index < players.size(); ++index) {
		Player &player = players[index];
		const PlayerId playerId = static_cast<PlayerId>(index);
		if (player.animationLock.active) {
			player.animationLock.elapsedSeconds += deltaSeconds;
				if (!player.animationLock.canCancel())
					continue;
				player.animationLock.active = false;
				EmitMovementEvent(eventSink_, MovementEventType::AnimationUnlocked, player.position.tile);
			}

		if (player.path.empty()) {
			player.moveState = player.destinationAction.type == DestinationActionType::None
			    ? PlayerMoveState::Idle
			    : PlayerMoveState::Acting;
			if (player.moveState == PlayerMoveState::Acting)
				actionExecutor_.update(player, playerId);
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
				EmitMovementEvent(eventSink_, MovementEventType::PathBlocked, nextTile);
				continue;
			}

		player.position.previous = player.position.tile;
			player.position.future = nextTile;
			player.position.tile = nextTile;
			player.position.precise = nextTile;
			EmitMovementEvent(eventSink_, MovementEventType::StepCommitted, nextTile);
			player.moveState = player.path.empty()
			    ? (player.destinationAction.type == DestinationActionType::None ? PlayerMoveState::Idle : PlayerMoveState::Acting)
			    : PlayerMoveState::Pathing;
			if (player.moveState == PlayerMoveState::Acting) {
				EmitMovementEvent(eventSink_, MovementEventType::DestinationActionReady, player.position.tile, player.destinationAction.type);
				actionExecutor_.update(player, playerId);
			}
	}
}

} // namespace dev
