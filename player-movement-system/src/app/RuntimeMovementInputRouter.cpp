#include "RuntimeMovementInputRouter.hpp"

namespace dev {

namespace {

bool IsPressedKey(const RawInputEvent &event, int code)
{
	return event.type == RawInputType::KeyPress && event.pressed && event.code == code;
}

RuntimeInputRouteResult QueuedMovement()
{
	return {
	    .handled = true,
	    .queuedMovementCommand = true,
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

	FocusState focusState = context.focusState;
	if (context.sessionMode == GameSessionMode::Paused)
		focusState.owner = InputOwner::Menu;
	else if (context.sessionMode == GameSessionMode::Inventory)
		focusState.owner = InputOwner::Inventory;

	InputFocus focus { focusState };
	PlayerActionGate gate { focus, context.actionContext };
	const Player &player = context.world->players[context.playerId];

	if (IsPressedKey(event, bindings_.stopKey)) {
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
	if (targetResult.handled)
		return targetResult;

	PlayerIntent intent = inputMapper_.mapToIntent(event, context.world->map, focus);
	std::optional<MovementCommand> command = commandBuilder_.buildMoveCommand(context.playerId, player, intent, gate);
	if (!command.has_value())
		return {};

	movementCommands_.enqueue(*command);
	return QueuedMovement();
}

} // namespace dev
