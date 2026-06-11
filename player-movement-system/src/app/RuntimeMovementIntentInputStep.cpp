#include "RuntimeMovementIntentInputStep.hpp"

#include "app/RuntimeInputRouteResultBuilder.hpp"

namespace dev {

RuntimeMovementIntentInputStep::RuntimeMovementIntentInputStep(QueuedMovementCommandSource &movementCommands)
    : movementCommands_(movementCommands)
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
	RuntimeInputRouteResultBuilder resultBuilder;
	PlayerIntent intent = inputMapper_.mapToIntent(event, map, focus);
	std::optional<MovementCommand> command = commandBuilder_.buildMoveCommand(playerId, player, intent, gate);
	if (!command.has_value())
		return resultBuilder.unhandled();

	movementCommands_.enqueue(*command);
	return resultBuilder.queuedMovementCommand();
}

} // namespace dev
