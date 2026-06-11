#pragma once

#include "app/RuntimeBlockedPointerInputStep.hpp"
#include "app/RuntimeInputTypes.hpp"
#include "app/RuntimeInputFocusResolver.hpp"
#include "app/RuntimeMovementIntentInputStep.hpp"
#include "app/RuntimeTargetInputRouter.hpp"
#include "app/RuntimeStopMovementInputStep.hpp"
#include "commands/MovementCommandSource.hpp"
#include "input/RawInput.hpp"

namespace dev {

class RuntimeMovementInputRouter {
public:
	RuntimeMovementInputRouter(QueuedMovementCommandSource &movementCommands, RuntimeInputBindings bindings = {});

	[[nodiscard]] RuntimeInputRouteResult route(const RawInputEvent &event, const RuntimeInputContext &context) const;

private:
	RuntimeInputBindings bindings_;
	RuntimeInputFocusResolver focusResolver_;
	RuntimeStopMovementInputStep stopInput_;
	RuntimeBlockedPointerInputStep blockedPointerInput_;
	RuntimeMovementIntentInputStep movementIntentInput_;
	RuntimeTargetInputRouter targetInput_;
};

} // namespace dev
