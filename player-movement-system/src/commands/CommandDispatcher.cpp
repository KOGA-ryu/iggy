#include "CommandDispatcher.hpp"

namespace dev {

CommandDispatcher::CommandDispatcher(PlayerController &playerController)
    : playerController_(playerController)
{
}

void CommandDispatcher::dispatch(const MovementCommand &command)
{
	switch (command.type) {
	case MovementCommandType::WalkTo:
		playerController_.walkTo(command.playerId, command.destination);
		break;
	case MovementCommandType::MoveThenAct:
		if (command.destinationAction.has_value())
			playerController_.moveThenAct(command.playerId, command.destination, *command.destinationAction);
		break;
	case MovementCommandType::StandAndAct:
		if (command.destinationAction.has_value())
			playerController_.standAndAct(command.playerId, *command.destinationAction);
		break;
	case MovementCommandType::Stop:
		playerController_.stop(command.playerId);
		break;
	}
}

} // namespace dev
