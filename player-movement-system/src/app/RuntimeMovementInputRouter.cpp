#include "RuntimeMovementInputRouter.hpp"

#include "input/InputEventMatcher.hpp"

namespace dev {

namespace {

RuntimeInputRouteResult QueuedMovement()
{
	return {
	    .handled = true,
	    .queuedMovementCommand = true,
	};
}

RuntimeInputRouteResult BlockedMovement(PlayerActionBlockReason reason)
{
	return {
	    .movementBlockReason = reason,
	};
}

} // namespace

RuntimeMovementInputRouter::RuntimeMovementInputRouter(QueuedMovementCommandSource &movementCommands, RuntimeInputBindings bindings)
    : movementCommands_(movementCommands)
    , bindings_(bindings)
    , targetInput_(movementCommands)
{
}

RuntimeInputRouteResult RuntimeMovementInputRouter::route(const RawInputEvent &event, const RuntimeInputContext &context) const
{
	if (context.world == nullptr || context.playerId >= context.world->players.size())
		return {};

	FocusState focusState = focusResolver_.resolve(context.focusState, context.sessionMode);
	InputFocus focus { focusState };
	PlayerActionGate gate { focus, context.actionContext };
	const Player &player = context.world->players[context.playerId];
	const PlayerActionBlockReason blockReason = gate.movementBlockReason(player);
	InputEventMatcher inputMatcher;

	if (inputMatcher.pressedKey(event, bindings_.stopKey)) {
		if (blockReason != PlayerActionBlockReason::None)
			return BlockedMovement(blockReason);
		std::optional<MovementCommand> stop = commandBuilder_.buildMoveCommand(
		    context.playerId,
		    player,
		    PlayerIntent { .type = PlayerIntentType::StopMoving },
		    gate);
		if (!stop.has_value())
			return {};
		movementCommands_.enqueue(*stop);
		return QueuedMovement();
	}

	RuntimeInputRouteResult targetResult = targetInput_.route(event, context, player, gate);
	if (targetResult.handled || targetResult.movementBlockReason.has_value())
		return targetResult;

	if (inputMatcher.pressedPointer(event) && blockReason != PlayerActionBlockReason::None)
		return BlockedMovement(blockReason);

	PlayerIntent intent = inputMapper_.mapToIntent(event, context.world->map, focus);
	std::optional<MovementCommand> command = commandBuilder_.buildMoveCommand(context.playerId, player, intent, gate);
	if (!command.has_value())
		return {};

	movementCommands_.enqueue(*command);
	return QueuedMovement();
}

} // namespace dev
