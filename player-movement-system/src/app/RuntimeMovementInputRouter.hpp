#pragma once

#include "app/RuntimeInputTypes.hpp"
#include "commands/IntentCommandBuilder.hpp"
#include "commands/MovementCommandSource.hpp"
#include "input/InputMapper.hpp"
#include "input/RawInput.hpp"
#include "app/RuntimeTargetInputRouter.hpp"

namespace dev {

class RuntimeMovementInputRouter {
public:
	RuntimeMovementInputRouter(QueuedMovementCommandSource &movementCommands, RuntimeInputBindings bindings = {});

	[[nodiscard]] RuntimeInputRouteResult route(const RawInputEvent &event, const RuntimeInputContext &context) const;

private:
	QueuedMovementCommandSource &movementCommands_;
	RuntimeInputBindings bindings_;
	InputMapper inputMapper_;
	IntentCommandBuilder commandBuilder_;
	RuntimeTargetInputRouter targetInput_;
};

} // namespace dev
