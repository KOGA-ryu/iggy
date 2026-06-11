#include "RuntimeInventoryScriptReportRecorder.hpp"

#include <utility>

namespace dev {

namespace {

void AppendCommandResults(
    const std::vector<InventoryScriptRunResult> &scriptResults,
    std::vector<InventoryCommandResult> &commandResults)
{
	for (const InventoryScriptRunResult &scriptResult : scriptResults) {
		commandResults.insert(
		    commandResults.end(),
		    scriptResult.commandResults.begin(),
		    scriptResult.commandResults.end());
	}
}

} // namespace

void RuntimeInventoryScriptReportRecorder::record(
    std::vector<InventoryScriptRunResult> scriptResults,
    RuntimeFrameReport &frame,
    RuntimeRunSummary &summary) const
{
	summary.runtimeInventoryScriptResults.insert(
	    summary.runtimeInventoryScriptResults.end(),
	    scriptResults.begin(),
	    scriptResults.end());
	AppendCommandResults(scriptResults, frame.inventoryCommandResults);
	AppendCommandResults(scriptResults, summary.inventoryCommandResults);
	frame.inventoryScriptResults = std::move(scriptResults);
}

} // namespace dev
