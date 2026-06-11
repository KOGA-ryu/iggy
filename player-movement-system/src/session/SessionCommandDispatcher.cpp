#include "SessionCommandDispatcher.hpp"

#include "session/SessionCommandApplier.hpp"
#include "session/SessionEventEmitter.hpp"

namespace dev {

SessionCommandDispatcher::SessionCommandDispatcher(GameSession &session, SessionEventSink *eventSink)
    : session_(session)
    , eventSink_(eventSink)
{
}

SessionCommandResult SessionCommandDispatcher::dispatch(const SessionCommand &command) const
{
	const SessionCommandApplication application = SessionCommandApplier { session_ }.apply(command);
	SessionEventEmitter { eventSink_ }.emit(command, application.eventType);
	return application.result;
}

} // namespace dev
