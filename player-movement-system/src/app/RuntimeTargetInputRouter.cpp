#include "RuntimeTargetInputRouter.hpp"

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

RuntimeTargetInputRouter::RuntimeTargetInputRouter(QueuedMovementCommandSource &movementCommands)
    : movementCommands_(movementCommands)
{
}

RuntimeInputRouteResult RuntimeTargetInputRouter::route(
    const RawInputEvent &event,
    const RuntimeInputContext &context,
    const Player &player,
    const PlayerActionGate &gate) const
{
	if (!InputEventMatcher {}.pressedPointer(event) || context.world == nullptr || context.targetResolver == nullptr)
		return {};
	const PlayerActionBlockReason blockReason = gate.movementBlockReason(player);
	if (blockReason != PlayerActionBlockReason::None)
		return BlockedMovement(blockReason);

	const Point tile = context.world->map.screenToTile(event.screenPosition);
	const Target target = context.targetResolver->resolveAtTile(tile);
	const InteractionIntent intent = interactionIntentBuilder_.build(target, player.movementModifiers.standGround);
	movementCommands_.enqueue(interactionCommandBuilder_.build(context.playerId, player, intent, gate));
	return QueuedMovement();
}

} // namespace dev
