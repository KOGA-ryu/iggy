#include "SimulationPlayerUpdater.hpp"

#include "actions/ActionExecutor.hpp"
#include "player/PlayerMovement.hpp"

namespace dev {

void SimulationPlayerUpdater::update(SimulationWorld &world, float deltaSeconds) const
{
	if (deltaSeconds <= 0.0F)
		return;

	ActionExecutor actionExecutor { ActionRules {}, world.movementEvents, &world.combat };
	PlayerMovement playerMovement { world.collision, actionExecutor, world.movementEvents };
	playerMovement.update(world.players, deltaSeconds);
}

} // namespace dev
