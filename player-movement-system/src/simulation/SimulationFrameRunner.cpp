#include "SimulationFrameRunner.hpp"

namespace dev {

SimulationFrameRunner::SimulationFrameRunner(SimulationClock *clock)
    : clock_(clock)
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
	targets_.syncEnemyTargets(world.enemies, world.combat.registry(), world.targets);
	targets_.removeDefeatedTargets(frameEvents.combatEvents(), world.targets);
	inventory_.applyPickupEvents(world, frameEvents.movementEvents());

	world.movementEvents = previousMovementEvents;
	world.setCombatEventSink(previousCombatEvents);

	EffectRouter router { frameEvents };
	for (const MovementEvent &event : frameEvents.movementEvents()) {
		router.route(event);
	}
	for (const CombatEvent &event : frameEvents.combatEvents()) {
		router.route(event);
	}

	EffectApplier applier { clock_ };
	for (const EffectRequest &request : frameEvents.effectRequests()) {
		applier.apply(request);
	}

	return frameEvents;
}

SimulationTimeStep SimulationFrameRunner::buildTimeStep(float rawDeltaSeconds) const
{
	if (clock_ != nullptr)
		return clock_->step(rawDeltaSeconds);
	return SimulationTimeStep::fromRawDelta(rawDeltaSeconds);
}

} // namespace dev
