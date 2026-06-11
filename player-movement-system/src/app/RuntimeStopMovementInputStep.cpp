#include "RuntimeStopMovementInputStep.hpp"

#include "app/RuntimeInputRouteResultBuilder.hpp"
#include "input/InputEventMatcher.hpp"

#include <optional>

namespace dev {

RuntimeStopMovementInputStep::RuntimeStopMovementInputStep(QueuedMovementCommandSource &movementCommands)
    : movementCommands_(movementCommands)
{
}

RuntimeInputRouteResult RuntimeStopMovementInputStep::route(
    const RawInputEvent &event,
    int stopKey,
    PlayerId playerId,
    const Player &player,
    const PlayerActionGate &gate,
    PlayerActionBlockReason blockReason) const
{
	RuntimeInputRouteResultBuilder resultBuilder;
	if (!InputEventMatcher {}.pressedKey(event, stopKey))
		return resultBuilder.unhandled();

	if (blockReason != PlayerActionBlockReason::None)
		return resultBuilder.blockedMovement(blockReason);

	std::optional<MovementCommand> stop = commandBuilder_.buildMoveCommand(
	    playerId,
	    player,
	    PlayerIntent { .type = PlayerIntentType::StopMoving },
	    gate);
	if (!stop.has_value())
		return resultBuilder.unhandled();

	movementCommands_.enqueue(*stop);
	return resultBuilder.queuedMovementCommand();
}

} // namespace dev
