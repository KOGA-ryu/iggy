#include "RuntimeInputRouter.hpp"

namespace dev {

RuntimeInputRouter::RuntimeInputRouter(
    QueuedSessionCommandSource &sessionCommands,
    QueuedMovementCommandSource &movementCommands,
    RuntimeInputBindings bindings)
    : sessionInput_(sessionCommands, bindings)
    , movementInput_(movementCommands, bindings)
{
}

RuntimeInputRouteResult RuntimeInputRouter::route(const RawInputEvent &event, const RuntimeInputContext &context) const
{
	RuntimeInputRouteResult sessionResult = sessionInput_.route(event, context);
	if (sessionResult.handled)
		return sessionResult;

	return movementInput_.route(event, context);
}

} // namespace dev
