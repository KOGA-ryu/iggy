#include "SimulationActorUpdater.hpp"

namespace dev {

void SimulationActorUpdater::update(SimulationWorld &world, const SimulationTimeStep &timeStep, const SimulationFramePolicy &policy) const
{
	if (policy.updatePlayers)
		players_.update(world, timeStep.playerDeltaSeconds);

	if (policy.updateEnemies)
		enemies_.update(world, timeStep.enemyDeltaSeconds);
}

} // namespace dev
