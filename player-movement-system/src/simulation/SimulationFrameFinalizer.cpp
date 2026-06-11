#include "SimulationFrameFinalizer.hpp"

namespace dev {

SimulationFrameFinalizer::SimulationFrameFinalizer(SimulationClock *clock)
    : effects_(clock)
{
}

void SimulationFrameFinalizer::finalize(SimulationWorld &world, SimulationFrameEvents &frameEvents) const
{
	targets_.finalize(world, frameEvents);
	inventory_.applyPickupEvents(world, frameEvents.movementEvents());

	effects_.run(frameEvents);
}

} // namespace dev
