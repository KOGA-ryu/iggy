#include "RuntimeSetupInventoryCommandReportRecorder.hpp"

namespace dev {

void RuntimeSetupInventoryCommandReportRecorder::record(
    const std::vector<InventoryCommandResult> &results,
    RuntimeRunSummary &summary) const
{
	summary.inventoryCommandResults.insert(
	    summary.inventoryCommandResults.end(),
	    results.begin(),
	    results.end());
}

} // namespace dev
