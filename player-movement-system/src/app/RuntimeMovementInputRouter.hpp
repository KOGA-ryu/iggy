#pragma once

#include "app/RuntimeInputTypes.hpp"
#include "app/RuntimeInputFocusResolver.hpp"
#include "app/RuntimeTargetInputRouter.hpp"
#include "commands/IntentCommandBuilder.hpp"
#include "commands/MovementCommandSource.hpp"
#include "input/InputMapper.hpp"
#include "input/RawInput.hpp"

namespace dev {

class RuntimeMovementInputRouter {
public:
	RuntimeMovementInputRouter(QueuedMovementCommandSource &movementCommands, RuntimeInputBindings bindings = {});

	[[nodiscard]] RuntimeInputRouteResult route(const RawInputEvent &event, const RuntimeInputContext &context) const;

private:
	QueuedMovementCommandSource &movementCommands_;
	RuntimeInputBindings bindings_;
	RuntimeInputFocusResolver focusResolver_;
	InputMapper inputMapper_;
	IntentCommandBuilder commandBuilder_;
	RuntimeTargetInputRouter targetInput_;
};

} // namespace dev
