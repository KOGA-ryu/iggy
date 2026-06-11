#include "IntentCommandBuilder.hpp"

namespace dev {

std::optional<MovementCommand> IntentCommandBuilder::buildMoveCommand(
    PlayerId playerId,
    const Player &player,
    const PlayerIntent &intent,
    const PlayerActionGate &gate) const
{
	if (!gate.canMove(player))
		return std::nullopt;

	if (intent.type == PlayerIntentType::MoveTo && intent.destination.has_value()) {
		return MovementCommand {
			.type = MovementCommandType::WalkTo,
			.playerId = playerId,
			.destination = *intent.destination,
		};
	}

	if (intent.type == PlayerIntentType::StopMoving) {
		return MovementCommand {
			.type = MovementCommandType::Stop,
			.playerId = playerId,
			.destination = player.position.tile,
		};
	}

	return std::nullopt;
}

} // namespace dev
