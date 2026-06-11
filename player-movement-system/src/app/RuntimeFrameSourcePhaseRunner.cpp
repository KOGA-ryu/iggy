#include "RuntimeFrameSourcePhaseRunner.hpp"

#include "app/RuntimeInventoryFrameSourceStep.hpp"

namespace dev {

void RuntimeFrameSourcePhaseRunner::run(
    RuntimeInputSourceRouter &inputSourceRouter,
    RuntimeSourceDrainer &sourceDrainer,
    RuntimeRunRecorder &recorder,
    const SessionCommandDispatcher &sessionDispatcher) const
{
	recorder.recordRawInputDrainResult(inputSourceRouter.route());

	recorder.recordSessionCommandResults(sourceDrainer.drainSessionCommands(sessionDispatcher));

	RuntimeInventoryFrameSourceStep {}.run(sourceDrainer, recorder);

	recorder.recordMovementScriptResults(sourceDrainer.drainMovementScripts());
	recorder.recordMovementCommandsQueued(sourceDrainer.drainMovementCommands());
}

} // namespace dev
