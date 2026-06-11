#include "SimulationCommandDrainer.hpp"

#include "commands/CommandDispatcher.hpp"
#include "player/PlayerController.hpp"

namespace dev {

int SimulationCommandDrainer::drain(SimulationWorld &world) const
{
	PlayerController playerController { world.players, world.map, world.collision, world.pathFinder, world.movementEvents };
	CommandDispatcher dispatcher { playerController, world.movementEvents };

	return queueDrain_.drain(world.commandQueue, dispatcher);
}

} // namespace dev
