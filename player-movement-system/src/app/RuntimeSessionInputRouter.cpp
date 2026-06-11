#include "RuntimeSessionInputRouter.hpp"

#include "app/RuntimeInputRouteResultBuilder.hpp"
#include "input/InputEventMatcher.hpp"

namespace dev {

RuntimeSessionInputRouter::RuntimeSessionInputRouter(QueuedSessionCommandSource &sessionCommands, RuntimeInputBindings bindings)
    : sessionCommands_(sessionCommands)
    , bindings_(bindings)
{
}

RuntimeInputRouteResult RuntimeSessionInputRouter::route(const RawInputEvent &event, const RuntimeInputContext &context) const
{
	RuntimeInputRouteResultBuilder resultBuilder;
	if (InputEventMatcher {}.pressedKey(event, bindings_.pauseKey)) {
		sessionCommands_.enqueue({
		    .type = SessionCommandType::SetMode,
		    .mode = modeTogglePolicy_.togglePause(context.sessionMode),
		});
		return resultBuilder.queuedSessionCommand();
	}

	if (InputEventMatcher {}.pressedKey(event, bindings_.inventoryKey)) {
		sessionCommands_.enqueue({
		    .type = SessionCommandType::SetMode,
		    .mode = modeTogglePolicy_.toggleInventory(context.sessionMode),
		});
		return resultBuilder.queuedSessionCommand();
	}

	return resultBuilder.unhandled();
}

} // namespace dev
