#include "SimulationEnemyUpdater.hpp"

#include "enemies/EnemyMovement.hpp"

namespace dev {

void SimulationEnemyUpdater::update(SimulationWorld &world, float deltaSeconds) const
{
	if (deltaSeconds <= 0.0F)
		return;

	Player *target = targetSelector_.selectTarget(world);
	if (target == nullptr)
		return;

	EnemyMovement enemyMovement { world.map, world.collision, world.movementEvents, &world.combat };
	enemyMovement.update(world.enemies, *target, deltaSeconds);
}

} // namespace dev
