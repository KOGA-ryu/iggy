#pragma once

#include "app/RuntimeMovementInputRouter.hpp"
#include "app/RuntimeInputTypes.hpp"
#include "app/RuntimeSessionInputRouter.hpp"
#include "commands/MovementCommandSource.hpp"
#include "input/RawInput.hpp"

namespace dev {

class RuntimeInputRouter {
public:
	RuntimeInputRouter(
	    QueuedSessionCommandSource &sessionCommands,
	    QueuedMovementCommandSource &movementCommands,
	    RuntimeInputBindings bindings = {});

	[[nodiscard]] RuntimeInputRouteResult route(const RawInputEvent &event, const RuntimeInputContext &context) const;

private:
	RuntimeSessionInputRouter sessionInput_;
	RuntimeMovementInputRouter movementInput_;
};

} // namespace dev
