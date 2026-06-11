#include "SimulationCommandDrainer.hpp"

#include "commands/CommandDispatcher.hpp"
#include "player/PlayerController.hpp"

namespace dev {

void SimulationCommandDrainer::drain(SimulationWorld &world) const
{
	PlayerController playerController { world.players, world.map, world.collision, world.pathFinder, world.movementEvents };
	CommandDispatcher dispatcher { playerController, world.movementEvents };

	MovementCommand command {};
	while (world.commandQueue.tryPop(command)) {
		dispatcher.dispatch(command);
	}
}

} // namespace dev
