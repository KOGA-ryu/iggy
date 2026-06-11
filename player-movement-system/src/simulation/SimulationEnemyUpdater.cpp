#include "SimulationEnemyUpdater.hpp"

#include "enemies/EnemyMovement.hpp"

namespace dev {

void SimulationEnemyUpdater::update(SimulationWorld &world, float deltaSeconds) const
{
	if (deltaSeconds <= 0.0F || world.players.empty())
		return;

	EnemyMovement enemyMovement { world.map, world.collision, world.movementEvents, &world.combat };
	enemyMovement.update(world.enemies, world.players.front(), deltaSeconds);
}

} // namespace dev
