#include "SimulationFrameFinalizer.hpp"

namespace dev {

SimulationFrameFinalizer::SimulationFrameFinalizer(SimulationClock *clock)
    : clock_(clock)
{
}

void SimulationFrameFinalizer::finalize(SimulationWorld &world, SimulationFrameEvents &frameEvents) const
{
	targets_.syncEnemyTargets(world.enemies, world.combat.registry(), world.targets);
	targets_.removeDefeatedTargets(frameEvents.combatEvents(), world.targets);
	inventory_.applyPickupEvents(world, frameEvents.movementEvents());

	routeEffects(frameEvents);
	applyEffects(frameEvents);
}

void SimulationFrameFinalizer::routeEffects(SimulationFrameEvents &frameEvents) const
{
	EffectRouter router { frameEvents };
	for (const MovementEvent &event : frameEvents.movementEvents()) {
		router.route(event);
	}
	for (const CombatEvent &event : frameEvents.combatEvents()) {
		router.route(event);
	}
}

void SimulationFrameFinalizer::applyEffects(const SimulationFrameEvents &frameEvents) const
{
	EffectApplier applier { clock_ };
	for (const EffectRequest &request : frameEvents.effectRequests()) {
		applier.apply(request);
	}
}

} // namespace dev
