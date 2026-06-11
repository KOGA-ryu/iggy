#include "RuntimeTargetInputRouter.hpp"

#include "app/RuntimeInputRouteResultBuilder.hpp"
#include "input/InputEventMatcher.hpp"

namespace dev {

RuntimeTargetInputRouter::RuntimeTargetInputRouter(QueuedMovementCommandSource &movementCommands)
    : targetInteractionInput_(movementCommands)
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

	return targetInteractionInput_.route(
	    event,
	    context.world->map,
	    *context.targetResolver,
	    context.playerId,
	    player,
	    gate);
}

} // namespace dev
