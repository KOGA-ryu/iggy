#pragma once

#include "app/RuntimeLoopTypes.hpp"

namespace dev {

class RuntimeInputDrainReportRecorder {
public:
	void record(
	    const RuntimeInputDrainResult &drainResult,
	    RuntimeFrameReport &frame,
	    RuntimeRunSummary &summary) const;
};

} // namespace dev
