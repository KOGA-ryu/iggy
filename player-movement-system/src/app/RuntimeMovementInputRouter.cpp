#include "RuntimeMovementInputRouter.hpp"

#include "app/RuntimeInputRouteResultBuilder.hpp"

namespace dev {

RuntimeMovementInputRouter::RuntimeMovementInputRouter(QueuedMovementCommandSource &movementCommands, RuntimeInputBindings bindings)
    : bindings_(bindings)
    , stopInput_(movementCommands)
    , movementIntentInput_(movementCommands)
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

	RuntimeInputRouteResult stopResult = stopInput_.route(
	    event,
	    bindings_.stopKey,
	    context.playerId,
	    player,
	    gate,
	    blockReason);
	if (stopResult.handled || stopResult.movementBlockReason.has_value())
		return stopResult;

	RuntimeInputRouteResult targetResult = targetInput_.route(event, context, player, gate);
	if (targetResult.handled || targetResult.movementBlockReason.has_value())
		return targetResult;

	RuntimeInputRouteResult blockedPointerResult = blockedPointerInput_.route(event, blockReason);
	if (blockedPointerResult.movementBlockReason.has_value())
		return blockedPointerResult;

	return movementIntentInput_.route(
	    event,
	    context.world->map,
	    context.playerId,
	    player,
	    focus,
	    gate);
}

} // namespace dev
