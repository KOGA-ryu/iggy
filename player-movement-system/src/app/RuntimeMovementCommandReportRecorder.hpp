#pragma once

#include "app/RuntimeLoopTypes.hpp"

namespace dev {

class RuntimeMovementCommandReportRecorder {
public:
	void recordQueuedCount(
	    int count,
	    RuntimeFrameReport &frame,
	    RuntimeRunSummary &summary) const;
};

} // namespace dev
