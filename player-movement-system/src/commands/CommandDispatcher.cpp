#include "CommandDispatcher.hpp"

namespace dev {

namespace {

void EmitCommandEvent(MovementEventSink *eventSink, MovementEventType type, const MovementCommand &command)
{
	if (eventSink == nullptr)
		return;
	eventSink->emit({
		.type = type,
		.playerId = command.playerId,
		.tile = command.destination,
		.commandType = command.type,
		.actionType = command.destinationAction.has_value()
		    ? std::optional<DestinationActionType> { command.destinationAction->type }
		    : std::nullopt,
	});
}

} // namespace

CommandDispatcher::CommandDispatcher(PlayerController &playerController, MovementEventSink *eventSink)
    : playerController_(playerController)
    , eventSink_(eventSink)
{
}

void CommandDispatcher::dispatch(const MovementCommand &command)
{
	switch (command.type) {
	case MovementCommandType::WalkTo:
		EmitCommandEvent(eventSink_, MovementEventType::CommandAccepted, command);
		playerController_.walkTo(command.playerId, command.destination);
		break;
	case MovementCommandType::MoveThenAct:
		if (command.destinationAction.has_value()) {
			EmitCommandEvent(eventSink_, MovementEventType::CommandAccepted, command);
			playerController_.moveThenAct(command.playerId, command.destination, *command.destinationAction);
		} else {
			EmitCommandEvent(eventSink_, MovementEventType::CommandRejected, command);
		}
		break;
	case MovementCommandType::StandAndAct:
		if (command.destinationAction.has_value()) {
			EmitCommandEvent(eventSink_, MovementEventType::CommandAccepted, command);
			playerController_.standAndAct(command.playerId, *command.destinationAction);
		} else {
			EmitCommandEvent(eventSink_, MovementEventType::CommandRejected, command);
		}
		break;
	case MovementCommandType::Stop:
		EmitCommandEvent(eventSink_, MovementEventType::CommandAccepted, command);
		playerController_.stop(command.playerId);
		break;
	}
}

} // namespace dev
