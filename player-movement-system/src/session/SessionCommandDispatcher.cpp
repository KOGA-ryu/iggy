#include "SessionCommandDispatcher.hpp"

namespace dev {

namespace {

SessionCommandResult Applied(const SessionCommand &command)
{
	return {
		.type = SessionCommandResultType::Applied,
		.command = command,
	};
}

SessionCommandResult Rejected(const SessionCommand &command)
{
	return {
		.type = SessionCommandResultType::Rejected,
		.command = command,
	};
}

} // namespace

SessionCommandDispatcher::SessionCommandDispatcher(GameSession &session, SessionEventSink *eventSink)
    : session_(session)
    , eventSink_(eventSink)
{
}

SessionCommandResult SessionCommandDispatcher::dispatch(const SessionCommand &command) const
{
	switch (command.type) {
	case SessionCommandType::StartNewGame:
		session_.startNewGame(command.newGameSettings.value_or(NewGameSettings {}));
		emit(command, SessionEventType::GameStarted);
		return Applied(command);
	case SessionCommandType::SaveSlot:
		if (!command.slotId.has_value() || !session_.saveToSlot(*command.slotId)) {
			emit(command, SessionEventType::SaveFailed);
			return Rejected(command);
		}
		emit(command, SessionEventType::SaveCompleted);
		return Applied(command);
	case SessionCommandType::LoadSlot:
		if (!command.slotId.has_value() || !session_.loadFromSlot(*command.slotId)) {
			emit(command, SessionEventType::LoadFailed);
			return Rejected(command);
		}
		emit(command, SessionEventType::LoadCompleted);
		return Applied(command);
	case SessionCommandType::SetMode:
		if (!command.mode.has_value()) {
			emit(command, SessionEventType::ModeChangeRejected);
			return Rejected(command);
		}
		const GameSessionMode before = session_.mode();
		session_.setMode(*command.mode);
		if (session_.mode() == *command.mode || before == *command.mode) {
			emit(command, SessionEventType::ModeChanged);
			return Applied(command);
		}
		emit(command, SessionEventType::ModeChangeRejected);
		return Rejected(command);
	}

	return Rejected(command);
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
