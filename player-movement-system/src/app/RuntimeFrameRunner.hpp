#pragma once

#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeRunRecorder.hpp"
#include "app/RuntimeSourceDrainer.hpp"
#include "commands/MovementCommandSource.hpp"
#include "session/GameSession.hpp"
#include "session/SessionCommandDispatcher.hpp"

namespace dev {

class RuntimeFrameRunner {
public:
	RuntimeFrameRunner(
	    GameSession &session,
	    QueuedSessionCommandSource &routedSessionCommands,
	    QueuedMovementCommandSource &routedMovementCommands,
	    RuntimeSourceDrainer &sourceDrainer,
	    RuntimeRunRecorder &recorder,
	    SessionCommandDispatcher &sessionDispatcher,
	    const RuntimeSourceSettings &sources,
	    const RuntimeInputSettings &input,
	    const RuntimeFrameSettings &frame);

	void runFrame();

private:
	[[nodiscard]] int routeRawInputSources();
	[[nodiscard]] SimulationFrameEvents updateSimulationFrame();
	void renderDebugView();

	GameSession &session_;
	QueuedSessionCommandSource &routedSessionCommands_;
	QueuedMovementCommandSource &routedMovementCommands_;
	RuntimeSourceDrainer &sourceDrainer_;
	RuntimeRunRecorder &recorder_;
	SessionCommandDispatcher &sessionDispatcher_;
	const RuntimeSourceSettings &sources_;
	const RuntimeInputSettings &input_;
	const RuntimeFrameSettings &frame_;
};

} // namespace dev
