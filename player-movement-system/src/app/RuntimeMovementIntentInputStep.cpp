#include "RuntimeMovementIntentInputStep.hpp"

namespace dev {

RuntimeMovementIntentInputStep::RuntimeMovementIntentInputStep(QueuedMovementCommandSource &movementCommands)
    : queueStep_(movementCommands)
{
}

RuntimeInputRouteResult RuntimeMovementIntentInputStep::route(
    const RawInputEvent &event,
    const TileMap &map,
    PlayerId playerId,
    const Player &player,
    const InputFocus &focus,
    const PlayerActionGate &gate) const
{
	PlayerIntent intent = inputMapper_.mapToIntent(event, map, focus);
	std::optional<MovementCommand> command = commandBuilder_.buildMoveCommand(playerId, player, intent, gate);
	return queueStep_.queue(command);
}

} // namespace dev
