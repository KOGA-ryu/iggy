#include "RuntimeFrameSourcePhaseRunner.hpp"

#include "app/RuntimeInventoryFrameSourceStep.hpp"
#include "app/RuntimeMovementFrameSourceStep.hpp"
#include "app/RuntimeSessionFrameSourceStep.hpp"

namespace dev {

void RuntimeFrameSourcePhaseRunner::run(
    RuntimeInputSourceRouter &inputSourceRouter,
    RuntimeSourceDrainer &sourceDrainer,
    RuntimeRunRecorder &recorder,
    const SessionCommandDispatcher &sessionDispatcher) const
{
	RuntimeSessionFrameSourceStep {}.run(
	    inputSourceRouter,
	    sourceDrainer,
	    recorder,
	    sessionDispatcher);

	RuntimeInventoryFrameSourceStep {}.run(sourceDrainer, recorder);

	RuntimeMovementFrameSourceStep {}.run(sourceDrainer, recorder);
}

} // namespace dev
