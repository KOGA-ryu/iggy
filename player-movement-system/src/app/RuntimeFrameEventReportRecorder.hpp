#pragma once

#include "app/RuntimeLoopTypes.hpp"

namespace dev {

class RuntimeFrameEventReportRecorder {
public:
	void record(
	    SimulationFrameEvents events,
	    RuntimeFrameReport &frame,
	    RuntimeRunSummary &summary) const;
};

} // namespace dev
