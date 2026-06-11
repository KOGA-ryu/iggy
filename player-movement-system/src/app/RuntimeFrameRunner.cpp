#include "RuntimeFrameRunner.hpp"

namespace dev {

RuntimeFrameRunner::RuntimeFrameRunner(
    GameSession &session,
    RuntimeInputSourceRouter &inputSourceRouter,
    RuntimeSourceDrainer &sourceDrainer,
    RuntimeRunRecorder &recorder,
    SessionCommandDispatcher &sessionDispatcher,
    const RuntimeFrameSettings &frame)
    : session_(session)
    , inputSourceRouter_(inputSourceRouter)
    , sourceDrainer_(sourceDrainer)
    , recorder_(recorder)
    , sessionDispatcher_(sessionDispatcher)
    , frame_(frame)
{
}

void RuntimeFrameRunner::runFrame()
{
	recorder_.beginFrame();

	recorder_.recordRawInputEventsRouted(inputSourceRouter_.route());

	recorder_.recordSessionCommandResults(sourceDrainer_.drainSessionCommands(sessionDispatcher_));

	recorder_.recordInventoryScriptResults(sourceDrainer_.drainInventoryScripts());
	recorder_.recordInventoryCommandResults(sourceDrainer_.drainInventoryCommands());

	recorder_.recordMovementCommandsQueued(sourceDrainer_.drainMovementCommands());

	recorder_.recordFrameEvents(updateSimulationFrame());
	recorder_.finishFrame();

	renderDebugView();
}

SimulationFrameEvents RuntimeFrameRunner::updateSimulationFrame()
{
	return session_.update(frame_.fixedDeltaSeconds);
}

void RuntimeFrameRunner::renderDebugView() {}

} // namespace dev
