#include "RuntimeTargetInputRouter.hpp"

#include "app/RuntimeInputRouteResultBuilder.hpp"
#include "input/InputEventMatcher.hpp"

namespace dev {

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
	RuntimeInputRouteResultBuilder resultBuilder;
	if (!InputEventMatcher {}.pressedPointer(event) || context.world == nullptr || context.targetResolver == nullptr)
		return resultBuilder.unhandled();
	const PlayerActionBlockReason blockReason = gate.movementBlockReason(player);
	if (blockReason != PlayerActionBlockReason::None)
		return resultBuilder.blockedMovement(blockReason);

	const Point tile = context.world->map.screenToTile(event.screenPosition);
	const Target target = context.targetResolver->resolveAtTile(tile);
	const InteractionIntent intent = interactionIntentBuilder_.build(target, player.movementModifiers.standGround);
	movementCommands_.enqueue(interactionCommandBuilder_.build(context.playerId, player, intent, gate));
	return resultBuilder.queuedMovementCommand();
}

} // namespace dev
