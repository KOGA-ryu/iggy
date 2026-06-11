#include "RuntimeFrameEventReportRecorder.hpp"

#include <utility>

namespace dev {

void RuntimeFrameEventReportRecorder::record(
    SimulationFrameEvents events,
    RuntimeFrameReport &frame,
    RuntimeRunSummary &summary) const
{
	frame.frameEvents = std::move(events);
	summary.lastFrameEvents = frame.frameEvents;
}

} // namespace dev
