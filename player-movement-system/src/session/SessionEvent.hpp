#pragma once

#include <optional>

#include "save/SaveSlot.hpp"
#include "session/GameSession.hpp"
#include "session/SessionCommand.hpp"

namespace dev {

enum class SessionEventType {
	GameStarted,
	SaveCompleted,
	SaveFailed,
	LoadCompleted,
	LoadFailed,
	ModeChanged,
	ModeChangeRejected,
};

struct SessionEvent {
	SessionEventType type = SessionEventType::ModeChangeRejected;
	SessionCommandType commandType = SessionCommandType::SetMode;
	std::optional<SaveSlotId> slotId;
	std::optional<GameSessionMode> mode;
};

} // namespace dev
