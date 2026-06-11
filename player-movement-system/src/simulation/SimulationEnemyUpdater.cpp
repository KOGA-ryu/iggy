#include "SimulationEnemyUpdater.hpp"

namespace dev {

void SimulationEnemyUpdater::update(SimulationWorld &world, float deltaSeconds) const
{
	if (deltaSeconds <= 0.0F)
		return;

	Player *target = targetSelector_.selectTarget(world);
	if (target == nullptr)
		return;

	movementRunner_.run(world, *target, deltaSeconds);
}

} // namespace dev
