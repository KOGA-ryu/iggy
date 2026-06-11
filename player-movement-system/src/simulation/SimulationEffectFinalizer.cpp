#include "SimulationEffectFinalizer.hpp"

namespace dev {

SimulationEffectFinalizer::SimulationEffectFinalizer(SimulationClock *clock)
    : pipeline_(clock)
{
}

void SimulationEffectFinalizer::finalize(SimulationFrameEvents &frameEvents) const
{
	pipeline_.run(frameEvents);
}

} // namespace dev
