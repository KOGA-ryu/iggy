#include "RuntimeTargetInputRouter.hpp"

namespace dev {

namespace {

bool IsPressedPointer(const RawInputEvent &event)
{
	return (event.type == RawInputType::MouseClick || event.type == RawInputType::TouchTap) && event.pressed;
}

RuntimeInputRouteResult QueuedMovement()
{
	return {
	    .handled = true,
	    .queuedMovementCommand = true,
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
	if (!IsPressedPointer(event) || context.world == nullptr || context.targetResolver == nullptr)
		return {};
	if (!gate.canMove(player))
		return {};

	const Point tile = context.world->map.screenToTile(event.screenPosition);
	const Target target = context.targetResolver->resolveAtTile(tile);
	const InteractionIntent intent = interactionIntentBuilder_.build(target, player.movementModifiers.standGround);
	movementCommands_.enqueue(interactionCommandBuilder_.build(context.playerId, player, intent, gate));
	return QueuedMovement();
}

} // namespace dev
