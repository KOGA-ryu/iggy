#include "RuntimeFrameRunner.hpp"

#include "app/RuntimeFramePolicyResolver.hpp"
#include "app/RuntimeSimulationFrameUpdater.hpp"

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

	recorder_.recordRawInputDrainResult(inputSourceRouter_.route());

	recorder_.recordSessionCommandResults(sourceDrainer_.drainSessionCommands(sessionDispatcher_));

	recorder_.recordInventoryScriptResults(sourceDrainer_.drainInventoryScripts());
	recorder_.recordInventoryCommandResults(sourceDrainer_.drainInventoryCommands());

	recorder_.recordMovementScriptResults(sourceDrainer_.drainMovementScripts());
	recorder_.recordMovementCommandsQueued(sourceDrainer_.drainMovementCommands());

	recorder_.recordFramePolicy(RuntimeFramePolicyResolver {}.resolve(session_));

	recorder_.recordFrameEvents(RuntimeSimulationFrameUpdater {}.update(session_, frame_));
	recorder_.finishFrame();

	renderDebugView();
}

void RuntimeFrameRunner::renderDebugView() {}

} // namespace dev
