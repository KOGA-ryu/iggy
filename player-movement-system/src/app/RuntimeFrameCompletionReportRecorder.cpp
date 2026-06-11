#include "RuntimeFrameCompletionReportRecorder.hpp"

#include <utility>

namespace dev {

void RuntimeFrameCompletionReportRecorder::record(
    std::vector<SessionEvent> sessionEvents,
    std::vector<InventoryEvent> inventoryEvents,
    RuntimeFrameReport frame,
    GameLoopResult &result) const
{
	frame.sessionEvents = std::move(sessionEvents);
	frame.inventoryEvents = std::move(inventoryEvents);
	result.frameReports.push_back(std::move(frame));
	++result.summary.framesRun;
}

} // namespace dev
