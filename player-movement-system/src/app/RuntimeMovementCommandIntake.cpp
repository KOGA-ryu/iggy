#include "RuntimeMovementCommandIntake.hpp"

namespace dev {

int RuntimeMovementCommandIntake::queue(
    std::vector<MovementCommand> commands,
    SimulationWorld &world) const
{
	for (MovementCommand command : commands)
		world.commandQueue.push(command);
	return static_cast<int>(commands.size());
}

} // namespace dev
