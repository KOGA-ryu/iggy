#include "RuntimeInventoryFrameSourceStep.hpp"

namespace dev {

void RuntimeInventoryFrameSourceStep::run(
    RuntimeSourceDrainer &sourceDrainer,
    RuntimeRunRecorder &recorder) const
{
	recorder.recordInventoryScriptResults(sourceDrainer.drainInventoryScripts());
	recorder.recordInventoryCommandResults(sourceDrainer.drainInventoryCommands());
}

} // namespace dev
