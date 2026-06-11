#pragma once

#include <optional>
#include <vector>

#include "focus/InputFocus.hpp"
#include "player/PlayerActionGate.hpp"
#include "session/GameSessionMode.hpp"
#include "simulation/SimulationWorld.hpp"
#include "targeting/TargetResolver.hpp"

namespace dev {

struct RuntimeInputBindings {
	int pauseKey = 27;
	int inventoryKey = 'I';
	int stopKey = 'S';
};

struct RuntimeInputContext {
	const SimulationWorld *world = nullptr;
	PlayerId playerId = 0;
	FocusState focusState;
	PlayerActionContext actionContext;
	GameSessionMode sessionMode = GameSessionMode::Empty;
	const TargetResolver *targetResolver = nullptr;
};

struct RuntimeInputRouteResult {
	bool handled = false;
	bool queuedSessionCommand = false;
	bool queuedMovementCommand = false;
	std::optional<PlayerActionBlockReason> movementBlockReason;
};

struct RuntimeInputDrainResult {
	int handled = 0;
	std::vector<PlayerActionBlockReason> movementBlockReasons;
};

} // namespace dev
