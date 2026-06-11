#include "SessionEventEmitter.hpp"

namespace dev {

SessionEventEmitter::SessionEventEmitter(SessionEventSink *eventSink)
    : eventSink_(eventSink)
{
}

void SessionEventEmitter::emit(const SessionCommand &command, SessionEventType type) const
{
	if (eventSink_ == nullptr)
		return;

	eventSink_->emit({
	    .type = type,
	    .commandType = command.type,
	    .slotId = command.slotId,
	    .mode = command.mode,
	});
}

} // namespace dev
