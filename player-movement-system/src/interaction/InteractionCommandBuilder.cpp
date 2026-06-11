#include "InteractionCommandBuilder.hpp"

namespace dev {

MovementCommand InteractionCommandBuilder::build(
    PlayerId playerId,
    const Player &player,
    const InteractionIntent &intent,
    const PlayerActionGate &gate) const
{
	if (!gate.canMove(player)) {
		return {
			.type = MovementCommandType::Stop,
			.playerId = playerId,
			.destination = player.position.tile,
		};
	}

	MovementCommandType commandType = MovementCommandType::WalkTo;
	if (intent.type == InteractionIntentType::Attack && player.movementModifiers.standGround)
		commandType = MovementCommandType::StandAndAct;
	else if (intent.type != InteractionIntentType::Move)
		commandType = MovementCommandType::MoveThenAct;

	return {
		.type = commandType,
		.playerId = playerId,
		.destination = intent.target.tile,
		.destinationAction = actions_.build(intent),
	};
}

} // namespace dev
