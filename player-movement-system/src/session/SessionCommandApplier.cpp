#include "SessionCommandApplier.hpp"

namespace dev {

SessionCommandApplier::SessionCommandApplier(GameSession &session)
    : session_(session)
{
}

SessionCommandApplication SessionCommandApplier::apply(const SessionCommand &command) const
{
	switch (command.type) {
	case SessionCommandType::StartNewGame:
		session_.startNewGame(command.newGameSettings.value_or(NewGameSettings {}));
		return applied(command, SessionEventType::GameStarted);
	case SessionCommandType::SaveSlot:
		if (!command.slotId.has_value() || !session_.saveToSlot(*command.slotId))
			return rejected(command, SessionEventType::SaveFailed);
		return applied(command, SessionEventType::SaveCompleted);
	case SessionCommandType::LoadSlot:
		if (!command.slotId.has_value() || !session_.loadFromSlot(*command.slotId))
			return rejected(command, SessionEventType::LoadFailed);
		return applied(command, SessionEventType::LoadCompleted);
	case SessionCommandType::SetMode:
		if (!command.mode.has_value())
			return rejected(command, SessionEventType::ModeChangeRejected);
		const GameSessionMode before = session_.mode();
		session_.setMode(*command.mode);
		if (session_.mode() == *command.mode || before == *command.mode)
			return applied(command, SessionEventType::ModeChanged);
		return rejected(command, SessionEventType::ModeChangeRejected);
	}

	return rejected(command, SessionEventType::ModeChangeRejected);
}

SessionCommandApplication SessionCommandApplier::applied(const SessionCommand &command, SessionEventType eventType) const
{
	return {
		.result = {
		    .type = SessionCommandResultType::Applied,
		    .command = command,
		},
		.eventType = eventType,
	};
}

SessionCommandApplication SessionCommandApplier::rejected(const SessionCommand &command, SessionEventType eventType) const
{
	return {
		.result = {
		    .type = SessionCommandResultType::Rejected,
		    .command = command,
		},
		.eventType = eventType,
	};
}

} // namespace dev
