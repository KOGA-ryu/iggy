#include "MovementCommandEventEmitter.hpp"

namespace dev {

MovementCommandEventEmitter::MovementCommandEventEmitter(MovementEventSink *eventSink)
    : eventSink_(eventSink)
{
}

void MovementCommandEventEmitter::accepted(const MovementCommand &command) const
{
	emit(MovementEventType::CommandAccepted, command);
}

void MovementCommandEventEmitter::rejected(const MovementCommand &command) const
{
	emit(MovementEventType::CommandRejected, command);
}

void MovementCommandEventEmitter::emit(MovementEventType type, const MovementCommand &command) const
{
	if (eventSink_ == nullptr)
		return;
	eventSink_->emit({
	    .type = type,
	    .playerId = command.playerId,
	    .tile = command.destination,
	    .commandType = command.type,
	    .actionType = command.destinationAction.has_value()
	        ? std::optional<DestinationActionType> { command.destinationAction->type }
	        : std::nullopt,
	});
}

} // namespace dev
