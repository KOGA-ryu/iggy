#pragma once

#include "session/GameSession.hpp"
#include "session/SessionCommand.hpp"
#include "session/SessionEvent.hpp"

namespace dev {

struct SessionCommandApplication {
	SessionCommandResult result;
	SessionEventType eventType = SessionEventType::ModeChangeRejected;
};

class SessionCommandApplier {
public:
	explicit SessionCommandApplier(GameSession &session);

	[[nodiscard]] SessionCommandApplication apply(const SessionCommand &command) const;

private:
	[[nodiscard]] SessionCommandApplication applied(const SessionCommand &command, SessionEventType eventType) const;
	[[nodiscard]] SessionCommandApplication rejected(const SessionCommand &command, SessionEventType eventType) const;

	GameSession &session_;
};

} // namespace dev
