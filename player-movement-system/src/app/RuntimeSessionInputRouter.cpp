#include "RuntimeSessionInputRouter.hpp"

namespace dev {

namespace {

bool IsPressedKey(const RawInputEvent &event, int code)
{
	return event.type == RawInputType::KeyPress && event.pressed && event.code == code;
}

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
	if (IsPressedKey(event, bindings_.pauseKey)) {
		sessionCommands_.enqueue({
		    .type = SessionCommandType::SetMode,
		    .mode = context.sessionMode == GameSessionMode::Paused ? GameSessionMode::Gameplay : GameSessionMode::Paused,
		});
		return QueuedSession();
	}

	if (IsPressedKey(event, bindings_.inventoryKey)) {
		sessionCommands_.enqueue({
		    .type = SessionCommandType::SetMode,
		    .mode = context.sessionMode == GameSessionMode::Inventory ? GameSessionMode::Gameplay : GameSessionMode::Inventory,
		});
		return QueuedSession();
	}

	return {};
}

} // namespace dev
