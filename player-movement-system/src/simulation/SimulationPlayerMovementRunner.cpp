#include "SimulationPlayerMovementRunner.hpp"

#include "actions/ActionExecutor.hpp"
#include "player/PlayerMovement.hpp"

namespace dev {

void SimulationPlayerMovementRunner::run(SimulationWorld &world, float deltaSeconds) const
{
	ActionExecutor actionExecutor { ActionRules {}, world.movementEvents, &world.combat };
	PlayerMovement playerMovement { world.collision, actionExecutor, world.movementEvents };
	playerMovement.update(world.players, deltaSeconds);
}

} // namespace dev
