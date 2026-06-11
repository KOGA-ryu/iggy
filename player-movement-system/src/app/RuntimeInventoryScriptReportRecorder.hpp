#pragma once

#include <vector>

#include "app/RuntimeLoopTypes.hpp"

namespace dev {

class RuntimeInventoryScriptReportRecorder {
public:
	void record(
	    std::vector<InventoryScriptRunResult> scriptResults,
	    RuntimeFrameReport &frame,
	    RuntimeRunSummary &summary) const;
};

} // namespace dev
