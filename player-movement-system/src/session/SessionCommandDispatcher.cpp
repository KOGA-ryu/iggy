#include "SessionCommandDispatcher.hpp"

#include "session/SessionCommandApplier.hpp"

namespace dev {

SessionCommandDispatcher::SessionCommandDispatcher(GameSession &session, SessionEventSink *eventSink)
    : session_(session)
    , eventSink_(eventSink)
{
}

SessionCommandResult SessionCommandDispatcher::dispatch(const SessionCommand &command) const
{
	const SessionCommandApplication application = SessionCommandApplier { session_ }.apply(command);
	emit(command, application.eventType);
	return application.result;
}

void SessionCommandDispatcher::emit(const SessionCommand &command, SessionEventType type) const
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
