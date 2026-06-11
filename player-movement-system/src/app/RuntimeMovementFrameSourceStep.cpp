#include "RuntimeMovementFrameSourceStep.hpp"

namespace dev {

void RuntimeMovementFrameSourceStep::run(
    RuntimeSourceDrainer &sourceDrainer,
    RuntimeRunRecorder &recorder) const
{
	recorder.recordMovementScriptResults(sourceDrainer.drainMovementScripts());
	recorder.recordMovementCommandsQueued(sourceDrainer.drainMovementCommands());
}

} // namespace dev
