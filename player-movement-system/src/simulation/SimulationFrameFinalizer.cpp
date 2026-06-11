#include "SimulationFrameFinalizer.hpp"

namespace dev {

SimulationFrameFinalizer::SimulationFrameFinalizer(SimulationClock *clock)
    : effects_(clock)
{
}

void SimulationFrameFinalizer::finalize(SimulationWorld &world, SimulationFrameEvents &frameEvents) const
{
	targets_.syncEnemyTargets(world.enemies, world.combat.registry(), world.targets);
	targets_.removeDefeatedTargets(frameEvents.combatEvents(), world.targets);
	inventory_.applyPickupEvents(world, frameEvents.movementEvents());

	effects_.run(frameEvents);
}

} // namespace dev
