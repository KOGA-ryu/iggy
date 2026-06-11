#pragma once

#include "commands/IntentCommandBuilder.hpp"
#include "commands/MovementCommandSource.hpp"
#include "focus/InputFocus.hpp"
#include "input/InputMapper.hpp"
#include "input/RawInput.hpp"
#include "interaction/InteractionCommandBuilder.hpp"
#include "interaction/InteractionIntentBuilder.hpp"
#include "player/PlayerActionGate.hpp"
#include "session/GameSession.hpp"
#include "session/SessionCommandSource.hpp"
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

class RuntimeInputRouter {
public:
	RuntimeInputRouter(
	    QueuedSessionCommandSource &sessionCommands,
	    QueuedMovementCommandSource &movementCommands,
	    RuntimeInputBindings bindings = {});

	[[nodiscard]] RuntimeInputRouteResult route(const RawInputEvent &event, const RuntimeInputContext &context) const;

private:
	[[nodiscard]] RuntimeInputRouteResult routeSessionInput(const RawInputEvent &event, const RuntimeInputContext &context) const;
	[[nodiscard]] RuntimeInputRouteResult routeMovementInput(const RawInputEvent &event, const RuntimeInputContext &context) const;

	QueuedSessionCommandSource &sessionCommands_;
	QueuedMovementCommandSource &movementCommands_;
	RuntimeInputBindings bindings_;
	InputMapper inputMapper_;
	IntentCommandBuilder commandBuilder_;
	InteractionIntentBuilder interactionIntentBuilder_;
	InteractionCommandBuilder interactionCommandBuilder_;
};

} // namespace dev
