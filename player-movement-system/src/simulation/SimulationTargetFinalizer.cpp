#include "SimulationTargetFinalizer.hpp"

namespace dev {

void SimulationTargetFinalizer::finalize(SimulationWorld &world, const SimulationFrameEvents &frameEvents) const
{
	targets_.syncEnemyTargets(world.enemies, world.combat.registry(), world.targets);
	targets_.removeDefeatedTargets(frameEvents.combatEvents(), world.targets);
}

} // namespace dev
