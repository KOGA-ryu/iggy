#include "RuntimeFramePolicyReportRecorder.hpp"

namespace dev {

void RuntimeFramePolicyReportRecorder::record(
    SimulationFramePolicyDescription description,
    RuntimeFrameReport &frame) const
{
	frame.framePolicy = description;
}

} // namespace dev
