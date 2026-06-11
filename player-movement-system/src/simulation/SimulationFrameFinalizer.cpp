#include "SimulationFrameFinalizer.hpp"

namespace dev {

SimulationFrameFinalizer::SimulationFrameFinalizer(SimulationClock *clock)
    : effects_(clock)
{
}

void SimulationFrameFinalizer::finalize(SimulationWorld &world, SimulationFrameEvents &frameEvents) const
{
	targets_.finalize(world, frameEvents);
	(void)inventory_.finalize(world, frameEvents);

	effects_.finalize(frameEvents);
}

} // namespace dev
