#pragma once

#include <vector>

#include "app/RuntimeLoopTypes.hpp"

namespace dev {

class RuntimeInventoryCommandReportRecorder {
public:
	void record(
	    const std::vector<InventoryCommandResult> &commandResults,
	    RuntimeFrameReport &frame,
	    RuntimeRunSummary &summary) const;
};

} // namespace dev
