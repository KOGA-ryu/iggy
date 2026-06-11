#include "RuntimeSessionCommandReportRecorder.hpp"

#include <utility>

namespace dev {

void RuntimeSessionCommandReportRecorder::record(
    std::vector<SessionCommandResult> commandResults,
    RuntimeFrameReport &frame,
    RuntimeRunSummary &summary) const
{
	summary.sessionCommandResults.insert(
	    summary.sessionCommandResults.end(),
	    commandResults.begin(),
	    commandResults.end());
	frame.sessionCommandResults = std::move(commandResults);
}

} // namespace dev
