#pragma once

#include <optional>

#include "app/RuntimeInputTypes.hpp"
#include "commands/MovementCommand.hpp"
#include "commands/MovementCommandSource.hpp"

namespace dev {

class RuntimeMovementCommandQueueStep {
public:
	explicit RuntimeMovementCommandQueueStep(QueuedMovementCommandSource &movementCommands);

	[[nodiscard]] RuntimeInputRouteResult queue(const MovementCommand &command) const;
	[[nodiscard]] RuntimeInputRouteResult queue(const std::optional<MovementCommand> &command) const;

private:
	QueuedMovementCommandSource &movementCommands_;
};

} // namespace dev
