#include "RuntimeMovementScriptReportRecorder.hpp"

#include <utility>

namespace dev {

void RuntimeMovementScriptReportRecorder::record(
    std::vector<MovementScriptRunResult> scriptResults,
    RuntimeFrameReport &frame,
    RuntimeRunSummary &summary) const
{
	summary.runtimeMovementScriptResults.insert(
	    summary.runtimeMovementScriptResults.end(),
	    scriptResults.begin(),
	    scriptResults.end());
	frame.movementScriptResults = std::move(scriptResults);
}

} // namespace dev
