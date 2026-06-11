#include "RuntimeMovementCommandReportRecorder.hpp"

namespace dev {

void RuntimeMovementCommandReportRecorder::recordQueuedCount(
    int count,
    RuntimeFrameReport &frame,
    RuntimeRunSummary &summary) const
{
	frame.movementCommandsQueued = count;
	summary.movementCommandsQueued += count;
}

} // namespace dev
