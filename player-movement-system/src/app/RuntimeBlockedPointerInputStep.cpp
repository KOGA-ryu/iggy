#include "RuntimeBlockedPointerInputStep.hpp"

#include "app/RuntimeInputRouteResultBuilder.hpp"
#include "input/InputEventMatcher.hpp"

namespace dev {

RuntimeInputRouteResult RuntimeBlockedPointerInputStep::route(
    const RawInputEvent &event,
    PlayerActionBlockReason blockReason) const
{
	RuntimeInputRouteResultBuilder resultBuilder;
	if (InputEventMatcher {}.pressedPointer(event) && blockReason != PlayerActionBlockReason::None)
		return resultBuilder.blockedMovement(blockReason);

	return resultBuilder.unhandled();
}

} // namespace dev
