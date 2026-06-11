#include "RuntimeInventoryCommandReportRecorder.hpp"

namespace dev {

void RuntimeInventoryCommandReportRecorder::record(
    const std::vector<InventoryCommandResult> &commandResults,
    RuntimeFrameReport &frame,
    RuntimeRunSummary &summary) const
{
	summary.inventoryCommandResults.insert(
	    summary.inventoryCommandResults.end(),
	    commandResults.begin(),
	    commandResults.end());
	frame.inventoryCommandResults.insert(
	    frame.inventoryCommandResults.end(),
	    commandResults.begin(),
	    commandResults.end());
}

} // namespace dev
