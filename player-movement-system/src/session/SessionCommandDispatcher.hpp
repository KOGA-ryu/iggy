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
	GameSession &session_;
	SessionEventSink *eventSink_;
};

} // namespace dev
