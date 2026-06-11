#pragma once

#include "app/RuntimeLoopTypes.hpp"

namespace dev {

class RuntimeFramePolicyReportRecorder {
public:
	void record(
	    SimulationFramePolicyDescription description,
	    RuntimeFrameReport &frame) const;
};

} // namespace dev
