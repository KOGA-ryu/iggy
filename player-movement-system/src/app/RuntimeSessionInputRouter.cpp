#include "RuntimeSessionInputRouter.hpp"

#include "input/InputEventMatcher.hpp"

namespace dev {

namespace {

RuntimeInputRouteResult QueuedSession()
{
	return {
	    .handled = true,
	    .queuedSessionCommand = true,
	};
}

} // namespace

RuntimeSessionInputRouter::RuntimeSessionInputRouter(QueuedSessionCommandSource &sessionCommands, RuntimeInputBindings bindings)
    : sessionCommands_(sessionCommands)
    , bindings_(bindings)
{
}

RuntimeInputRouteResult RuntimeSessionInputRouter::route(const RawInputEvent &event, const RuntimeInputContext &context) const
{
	if (InputEventMatcher {}.pressedKey(event, bindings_.pauseKey)) {
		sessionCommands_.enqueue({
		    .type = SessionCommandType::SetMode,
		    .mode = modeTogglePolicy_.togglePause(context.sessionMode),
		});
		return QueuedSession();
	}

	if (InputEventMatcher {}.pressedKey(event, bindings_.inventoryKey)) {
		sessionCommands_.enqueue({
		    .type = SessionCommandType::SetMode,
		    .mode = modeTogglePolicy_.toggleInventory(context.sessionMode),
		});
		return QueuedSession();
	}

	return {};
}

} // namespace dev
