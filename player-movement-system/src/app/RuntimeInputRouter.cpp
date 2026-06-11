#include "RuntimeInputRouter.hpp"

namespace dev {

namespace {

bool IsPressedKey(const RawInputEvent &event, int code)
{
	return event.type == RawInputType::KeyPress && event.pressed && event.code == code;
}

bool IsPressedPointer(const RawInputEvent &event)
{
	return (event.type == RawInputType::MouseClick || event.type == RawInputType::TouchTap) && event.pressed;
}

RuntimeInputRouteResult QueuedSession()
{
	return {
		.handled = true,
		.queuedSessionCommand = true,
	};
}

RuntimeInputRouteResult QueuedMovement()
{
	return {
		.handled = true,
		.queuedMovementCommand = true,
	};
}

} // namespace

RuntimeInputRouter::RuntimeInputRouter(
    QueuedSessionCommandSource &sessionCommands,
    QueuedMovementCommandSource &movementCommands,
    RuntimeInputBindings bindings)
    : sessionCommands_(sessionCommands)
    , movementCommands_(movementCommands)
    , bindings_(bindings)
{
}

RuntimeInputRouteResult RuntimeInputRouter::route(const RawInputEvent &event, const RuntimeInputContext &context) const
{
	RuntimeInputRouteResult sessionResult = routeSessionInput(event, context);
	if (sessionResult.handled)
		return sessionResult;

	return routeMovementInput(event, context);
}

RuntimeInputRouteResult RuntimeInputRouter::routeSessionInput(const RawInputEvent &event, const RuntimeInputContext &context) const
{
	if (IsPressedKey(event, bindings_.pauseKey)) {
		sessionCommands_.enqueue({
		    .type = SessionCommandType::SetMode,
		    .mode = context.sessionMode == GameSessionMode::Paused ? GameSessionMode::Gameplay : GameSessionMode::Paused,
		});
		return QueuedSession();
	}

	if (IsPressedKey(event, bindings_.inventoryKey)) {
		sessionCommands_.enqueue({
		    .type = SessionCommandType::SetMode,
		    .mode = context.sessionMode == GameSessionMode::Inventory ? GameSessionMode::Gameplay : GameSessionMode::Inventory,
		});
		return QueuedSession();
	}

	return {};
}

RuntimeInputRouteResult RuntimeInputRouter::routeMovementInput(const RawInputEvent &event, const RuntimeInputContext &context) const
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

	if (IsPressedPointer(event) && context.targetResolver != nullptr) {
		if (!gate.canMove(player))
			return {};
		const Point tile = context.world->map.screenToTile(event.screenPosition);
		const Target target = context.targetResolver->resolveAtTile(tile);
		const InteractionIntent intent = interactionIntentBuilder_.build(target, player.movementModifiers.standGround);
		movementCommands_.enqueue(interactionCommandBuilder_.build(context.playerId, player, intent, gate));
		return QueuedMovement();
	}

	PlayerIntent intent = inputMapper_.mapToIntent(event, context.world->map, focus);
	std::optional<MovementCommand> command = commandBuilder_.buildMoveCommand(context.playerId, player, intent, gate);
	if (!command.has_value())
		return {};

	movementCommands_.enqueue(*command);
	return QueuedMovement();
}

} // namespace dev
