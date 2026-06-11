#include "RuntimeSessionFrameSourceStep.hpp"

namespace dev {

void RuntimeSessionFrameSourceStep::run(
    RuntimeInputSourceRouter &inputSourceRouter,
    RuntimeSourceDrainer &sourceDrainer,
    RuntimeRunRecorder &recorder,
    const SessionCommandDispatcher &sessionDispatcher) const
{
	recorder.recordRawInputDrainResult(inputSourceRouter.route());
	recorder.recordSessionCommandResults(sourceDrainer.drainSessionCommands(sessionDispatcher));
}

} // namespace dev
