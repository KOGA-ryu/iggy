#include "SimulationFrameRunner.hpp"

namespace dev {

SimulationFrameRunner::SimulationFrameRunner(SimulationClock *clock)
    : timeSteps_(clock)
    , finalizer_(clock)
{
}

SimulationFrameEvents SimulationFrameRunner::run(SimulationWorld &world, float rawDeltaSeconds) const
{
	return run(world, timeSteps_.build(rawDeltaSeconds));
}

SimulationFrameEvents SimulationFrameRunner::run(SimulationWorld &world, float rawDeltaSeconds, const SimulationFramePolicy &policy) const
{
	return run(world, timeSteps_.build(rawDeltaSeconds), policy);
}

SimulationFrameEvents SimulationFrameRunner::run(SimulationWorld &world, const SimulationTimeStep &timeStep) const
{
	return run(world, timeStep, SimulationFramePolicy::forMode(SimulationMode::Gameplay));
}

SimulationFrameEvents SimulationFrameRunner::run(SimulationWorld &world, const SimulationTimeStep &timeStep, const SimulationFramePolicy &policy) const
{
	SimulationFrameEvents frameEvents = ticks_.run(world, timeStep, policy);
	finalizer_.finalize(world, frameEvents);

	return frameEvents;
}

} // namespace dev
