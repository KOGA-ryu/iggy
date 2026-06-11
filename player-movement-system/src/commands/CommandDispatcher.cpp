#include "CommandDispatcher.hpp"

namespace dev {

CommandDispatcher::CommandDispatcher(PlayerController &playerController, MovementEventSink *eventSink)
    : playerController_(playerController)
    , events_(eventSink)
{
}

MovementCommandDispatchResult CommandDispatcher::dispatch(const MovementCommand &command)
{
	if (!validator_.accepts(command)) {
		events_.rejected(command);
		return {
		    .type = MovementCommandDispatchResultType::Rejected,
		    .command = command,
		};
	}

	events_.accepted(command);

	switch (command.type) {
	case MovementCommandType::WalkTo:
		playerController_.walkTo(command.playerId, command.destination);
		break;
	case MovementCommandType::MoveThenAct:
		playerController_.moveThenAct(command.playerId, command.destination, *command.destinationAction);
		break;
	case MovementCommandType::StandAndAct:
		playerController_.standAndAct(command.playerId, *command.destinationAction);
		break;
	case MovementCommandType::Stop:
		playerController_.stop(command.playerId);
		break;
	}

	return {
	    .type = MovementCommandDispatchResultType::Accepted,
	    .command = command,
	};
}

} // namespace dev
