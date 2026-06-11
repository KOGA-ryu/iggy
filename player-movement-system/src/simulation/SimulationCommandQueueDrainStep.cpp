#include "SimulationCommandQueueDrainStep.hpp"

namespace dev {

int SimulationCommandQueueDrainStep::drain(CommandQueue &commands, CommandDispatcher &dispatcher) const
{
	int drained = 0;
	MovementCommand command {};
	while (commands.tryPop(command)) {
		dispatcher.dispatch(command);
		++drained;
	}
	return drained;
}

} // namespace dev
