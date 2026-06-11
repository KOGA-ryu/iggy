#pragma once

#include <vector>

#include "app/RuntimeLoopTypes.hpp"
#include "inventory/InventoryCommand.hpp"

namespace dev {

class RuntimeSetupInventoryCommandReportRecorder {
public:
	void record(
	    const std::vector<InventoryCommandResult> &results,
	    RuntimeRunSummary &summary) const;
};

} // namespace dev
