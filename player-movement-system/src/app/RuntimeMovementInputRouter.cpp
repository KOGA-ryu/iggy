#include "RuntimeMovementInputRouter.hpp"

#include "app/RuntimeInputRouteResultBuilder.hpp"
#include "input/InputEventMatcher.hpp"

namespace dev {

RuntimeMovementInputRouter::RuntimeMovementInputRouter(QueuedMovementCommandSource &movementCommands, RuntimeInputBindings bindings)
    : movementCommands_(movementCommands)
    , bindings_(bindings)
    , targetInput_(movementCommands)
{
}

RuntimeInputRouteResult RuntimeMovementInputRouter::route(const RawInputEvent &event, const RuntimeInputContext &context) const
{
	RuntimeInputRouteResultBuilder resultBuilder;
	if (context.world == nullptr || context.playerId >= context.world->players.size())
		return resultBuilder.unhandled();

	FocusState focusState = focusResolver_.resolve(context.focusState, context.sessionMode);
	InputFocus focus { focusState };
	PlayerActionGate gate { focus, context.actionContext };
	const Player &player = context.world->players[context.playerId];
	const PlayerActionBlockReason blockReason = gate.movementBlockReason(player);
	InputEventMatcher inputMatcher;

	if (inputMatcher.pressedKey(event, bindings_.stopKey)) {
		if (blockReason != PlayerActionBlockReason::None)
			return resultBuilder.blockedMovement(blockReason);
		std::optional<MovementCommand> stop = commandBuilder_.buildMoveCommand(
		    context.playerId,
		    player,
		    PlayerIntent { .type = PlayerIntentType::StopMoving },
		    gate);
		if (!stop.has_value())
			return resultBuilder.unhandled();
		movementCommands_.enqueue(*stop);
		return resultBuilder.queuedMovementCommand();
	}

	RuntimeInputRouteResult targetResult = targetInput_.route(event, context, player, gate);
	if (targetResult.handled || targetResult.movementBlockReason.has_value())
		return targetResult;

	if (inputMatcher.pressedPointer(event) && blockReason != PlayerActionBlockReason::None)
		return resultBuilder.blockedMovement(blockReason);

	PlayerIntent intent = inputMapper_.mapToIntent(event, context.world->map, focus);
	std::optional<MovementCommand> command = commandBuilder_.buildMoveCommand(context.playerId, player, intent, gate);
	if (!command.has_value())
		return resultBuilder.unhandled();

	movementCommands_.enqueue(*command);
	return resultBuilder.queuedMovementCommand();
}

} // namespace dev
