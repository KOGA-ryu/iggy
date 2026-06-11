#pragma once

#include "focus/InputFocus.hpp"
#include "player/PlayerActionGate.hpp"
#include "session/GameSession.hpp"
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
};

} // namespace dev
