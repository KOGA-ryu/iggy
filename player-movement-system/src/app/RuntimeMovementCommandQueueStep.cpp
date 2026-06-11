#include "RuntimeMovementCommandQueueStep.hpp"

#include "app/RuntimeInputRouteResultBuilder.hpp"

namespace dev {

RuntimeMovementCommandQueueStep::RuntimeMovementCommandQueueStep(QueuedMovementCommandSource &movementCommands)
    : movementCommands_(movementCommands)
{
}

RuntimeInputRouteResult RuntimeMovementCommandQueueStep::queue(const MovementCommand &command) const
{
	movementCommands_.enqueue(command);
	return RuntimeInputRouteResultBuilder {}.queuedMovementCommand();
}

RuntimeInputRouteResult RuntimeMovementCommandQueueStep::queue(const std::optional<MovementCommand> &command) const
{
	if (!command.has_value())
		return RuntimeInputRouteResultBuilder {}.unhandled();

	return queue(*command);
}

} // namespace dev
