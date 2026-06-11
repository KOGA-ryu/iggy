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
		.destinationAction = destinationActionFor(intent),
	};
}

DestinationAction InteractionCommandBuilder::destinationActionFor(const InteractionIntent &intent) const
{
	switch (intent.type) {
	case InteractionIntentType::Attack:
		return { DestinationActionType::Attack, intent.target, 1 };
	case InteractionIntentType::Pickup:
		return { DestinationActionType::Pickup, intent.target, 0 };
	case InteractionIntentType::Talk:
		return { DestinationActionType::Talk, intent.target, 1 };
	case InteractionIntentType::Interact:
		return { DestinationActionType::Interact, intent.target, 1 };
	case InteractionIntentType::Move:
		return {};
	}
	return {};
}

} // namespace dev

