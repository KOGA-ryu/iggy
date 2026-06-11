#pragma once

#include <optional>

#include "save/SaveSlot.hpp"
#include "session/GameSessionMode.hpp"
#include "session/NewGameSettings.hpp"

namespace dev {

enum class SessionCommandType {
	StartNewGame,
	SaveSlot,
	LoadSlot,
	SetMode,
};

struct SessionCommand {
	SessionCommandType type = SessionCommandType::StartNewGame;
	std::optional<NewGameSettings> newGameSettings;
	std::optional<SaveSlotId> slotId;
	std::optional<GameSessionMode> mode;
};

enum class SessionCommandResultType {
	Applied,
	Rejected,
};

struct SessionCommandResult {
	SessionCommandResultType type = SessionCommandResultType::Rejected;
	SessionCommand command;
};

} // namespace dev
