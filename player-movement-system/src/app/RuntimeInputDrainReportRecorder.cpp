#include "RuntimeInputDrainReportRecorder.hpp"

namespace dev {

void RuntimeInputDrainReportRecorder::record(
    const RuntimeInputDrainResult &drainResult,
    RuntimeFrameReport &frame,
    RuntimeRunSummary &summary) const
{
	frame.rawInputEventsRouted = drainResult.handled;
	summary.rawInputEventsRouted += drainResult.handled;
	frame.movementInputBlockReasons = drainResult.movementBlockReasons;
	summary.movementInputBlockReasons.insert(
	    summary.movementInputBlockReasons.end(),
	    drainResult.movementBlockReasons.begin(),
	    drainResult.movementBlockReasons.end());
}

} // namespace dev
