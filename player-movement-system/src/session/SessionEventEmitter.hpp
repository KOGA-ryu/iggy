#pragma once

#include "session/SessionCommand.hpp"
#include "session/SessionEventSink.hpp"

namespace dev {

class SessionEventEmitter {
public:
	explicit SessionEventEmitter(SessionEventSink *eventSink = nullptr);

	void emit(const SessionCommand &command, SessionEventType type) const;

private:
	SessionEventSink *eventSink_;
};

} // namespace dev
