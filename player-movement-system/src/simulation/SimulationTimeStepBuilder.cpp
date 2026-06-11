#include "SimulationTimeStepBuilder.hpp"

namespace dev {

SimulationTimeStepBuilder::SimulationTimeStepBuilder(SimulationClock *clock)
    : clock_(clock)
{
}

SimulationTimeStep SimulationTimeStepBuilder::build(float rawDeltaSeconds) const
{
	if (clock_ != nullptr)
		return clock_->step(rawDeltaSeconds);
	return SimulationTimeStep::fromRawDelta(rawDeltaSeconds);
}

} // namespace dev
