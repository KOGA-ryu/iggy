#include "SimulationPlayerUpdater.hpp"

namespace dev {

void SimulationPlayerUpdater::update(SimulationWorld &world, float deltaSeconds) const
{
	if (deltaSeconds <= 0.0F)
		return;

	movementRunner_.run(world, deltaSeconds);
}

} // namespace dev
