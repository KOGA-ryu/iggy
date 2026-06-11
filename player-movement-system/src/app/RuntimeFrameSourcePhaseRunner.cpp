#include "RuntimeFrameSourcePhaseRunner.hpp"

namespace dev {

void RuntimeFrameSourcePhaseRunner::run(
    RuntimeInputSourceRouter &inputSourceRouter,
    RuntimeSourceDrainer &sourceDrainer,
    RuntimeRunRecorder &recorder,
    const SessionCommandDispatcher &sessionDispatcher) const
{
	recorder.recordRawInputDrainResult(inputSourceRouter.route());

	recorder.recordSessionCommandResults(sourceDrainer.drainSessionCommands(sessionDispatcher));

	recorder.recordInventoryScriptResults(sourceDrainer.drainInventoryScripts());
	recorder.recordInventoryCommandResults(sourceDrainer.drainInventoryCommands());

	recorder.recordMovementScriptResults(sourceDrainer.drainMovementScripts());
	recorder.recordMovementCommandsQueued(sourceDrainer.drainMovementCommands());
}

} // namespace dev
