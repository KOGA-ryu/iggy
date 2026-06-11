#include "SimulationEnemyMovementRunner.hpp"

#include "enemies/EnemyMovement.hpp"

namespace dev {

void SimulationEnemyMovementRunner::run(SimulationWorld &world, Player &target, float deltaSeconds) const
{
	EnemyMovement enemyMovement { world.map, world.collision, world.movementEvents, &world.combat };
	enemyMovement.update(world.enemies, target, deltaSeconds);
}

} // namespace dev
