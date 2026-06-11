#include "RuntimeTargetInteractionInputStep.hpp"

namespace dev {

RuntimeTargetInteractionInputStep::RuntimeTargetInteractionInputStep(QueuedMovementCommandSource &movementCommands)
    : queueStep_(movementCommands)
{
}

RuntimeInputRouteResult RuntimeTargetInteractionInputStep::route(
    const RawInputEvent &event,
    const TileMap &map,
    const TargetResolver &targetResolver,
    PlayerId playerId,
    const Player &player,
    const PlayerActionGate &gate) const
{
	const Point tile = map.screenToTile(event.screenPosition);
	const Target target = targetResolver.resolveAtTile(tile);
	const InteractionIntent intent = interactionIntentBuilder_.build(target, player.movementModifiers.standGround);
	return queueStep_.queue(interactionCommandBuilder_.build(playerId, player, intent, gate));
}

} // namespace dev
