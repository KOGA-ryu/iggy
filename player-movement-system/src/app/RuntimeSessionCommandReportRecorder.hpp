#pragma once

#include <vector>

#include "app/RuntimeLoopTypes.hpp"

namespace dev {

class RuntimeSessionCommandReportRecorder {
public:
	void record(
	    std::vector<SessionCommandResult> commandResults,
	    RuntimeFrameReport &frame,
	    RuntimeRunSummary &summary) const;
};

} // namespace dev
