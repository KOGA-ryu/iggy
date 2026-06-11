#pragma once

#include "app/RuntimeLoopTypes.hpp"
#include "commands/MovementCommandSource.hpp"
#include "session/GameSession.hpp"
#include "session/SessionCommandSource.hpp"

namespace dev {

class RuntimeInputSourceRouter {
public:
	RuntimeInputSourceRouter(
	    const GameSession &session,
	    QueuedSessionCommandSource &routedSessionCommands,
	    QueuedMovementCommandSource &routedMovementCommands,
	    const RuntimeSourceSettings &sources,
	    const RuntimeInputSettings &input);

	[[nodiscard]] RuntimeInputDrainResult route();

private:
	const GameSession &session_;
	QueuedSessionCommandSource &routedSessionCommands_;
	QueuedMovementCommandSource &routedMovementCommands_;
	const RuntimeSourceSettings &sources_;
	const RuntimeInputSettings &input_;
};

} // namespace dev
