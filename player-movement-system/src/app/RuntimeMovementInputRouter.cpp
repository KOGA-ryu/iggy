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
	std::optional<RuntimeMovementInputContext> movementContext = contextBuilder_.build(context);
	if (!movementContext.has_value())
		return resultBuilder.unhandled();

	InputFocus focus { movementContext->focusState };
	PlayerActionGate gate { focus, movementContext->actionContext };

	RuntimeInputRouteResult stopResult = stopInput_.route(
	    event,
	    bindings_.stopKey,
	    context.playerId,
	    *movementContext->player,
	    gate,
	    movementContext->blockReason);
	if (stopResult.handled || stopResult.movementBlockReason.has_value())
		return stopResult;

	RuntimeInputRouteResult targetResult = targetInput_.route(event, context, *movementContext->player, gate);
	if (targetResult.handled || targetResult.movementBlockReason.has_value())
		return targetResult;

	RuntimeInputRouteResult blockedPointerResult = blockedPointerInput_.route(event, movementContext->blockReason);
	if (blockedPointerResult.movementBlockReason.has_value())
		return blockedPointerResult;

	return movementIntentInput_.route(
	    event,
	    movementContext->world->map,
	    context.playerId,
	    *movementContext->player,
	    focus,
	    gate);
}

} // namespace dev
