#pragma once

#include "session/GameSession.hpp"
#include "session/SessionCommand.hpp"
#include "session/SessionEventSink.hpp"

namespace dev {

class SessionCommandDispatcher {
public:
	explicit SessionCommandDispatcher(GameSession &session, SessionEventSink *eventSink = nullptr);

	[[nodiscard]] SessionCommandResult dispatch(const SessionCommand &command) const;

private:
	void emit(const SessionCommand &command, SessionEventType type) const;

	GameSession &session_;
	SessionEventSink *eventSink_;
};

} // namespace dev
