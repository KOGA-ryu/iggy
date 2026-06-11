#include "SimulationFrameRunner.hpp"

namespace dev {

SimulationFrameRunner::SimulationFrameRunner(SimulationClock *clock)
    : clock_(clock)
    , finalizer_(clock)
{
}

SimulationFrameEvents SimulationFrameRunner::run(SimulationWorld &world, float rawDeltaSeconds) const
{
	return run(world, buildTimeStep(rawDeltaSeconds));
}

SimulationFrameEvents SimulationFrameRunner::run(SimulationWorld &world, float rawDeltaSeconds, const SimulationFramePolicy &policy) const
{
	return run(world, buildTimeStep(rawDeltaSeconds), policy);
}

SimulationFrameEvents SimulationFrameRunner::run(SimulationWorld &world, const SimulationTimeStep &timeStep) const
{
	return run(world, timeStep, SimulationFramePolicy::forMode(SimulationMode::Gameplay));
}

SimulationFrameEvents SimulationFrameRunner::run(SimulationWorld &world, const SimulationTimeStep &timeStep, const SimulationFramePolicy &policy) const
{
	MovementEventSink *previousMovementEvents = world.movementEvents;
	CombatEventSink *previousCombatEvents = world.combatEvents;
	SimulationFrameEvents frameEvents { previousMovementEvents, previousCombatEvents };

	world.movementEvents = &frameEvents;
	world.setCombatEventSink(&frameEvents);

	tick_.update(world, timeStep, policy);

	world.movementEvents = previousMovementEvents;
	world.setCombatEventSink(previousCombatEvents);

	finalizer_.finalize(world, frameEvents);

	return frameEvents;
}

SimulationTimeStep SimulationFrameRunner::buildTimeStep(float rawDeltaSeconds) const
{
	if (clock_ != nullptr)
		return clock_->step(rawDeltaSeconds);
	return SimulationTimeStep::fromRawDelta(rawDeltaSeconds);
}

} // namespace dev
