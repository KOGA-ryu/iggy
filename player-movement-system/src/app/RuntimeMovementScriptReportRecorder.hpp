#pragma once

#include <vector>

#include "app/RuntimeLoopTypes.hpp"

namespace dev {

class RuntimeMovementScriptReportRecorder {
public:
	void record(
	    std::vector<MovementScriptRunResult> scriptResults,
	    RuntimeFrameReport &frame,
	    RuntimeRunSummary &summary) const;
};

} // namespace dev
