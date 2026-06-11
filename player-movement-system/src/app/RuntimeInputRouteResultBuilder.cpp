#include "RuntimeInputRouteResultBuilder.hpp"

namespace dev {

RuntimeInputRouteResult RuntimeInputRouteResultBuilder::unhandled() const
{
	return {};
}

RuntimeInputRouteResult RuntimeInputRouteResultBuilder::queuedSessionCommand() const
{
	return {
	    .handled = true,
	    .queuedSessionCommand = true,
	};
}

RuntimeInputRouteResult RuntimeInputRouteResultBuilder::queuedMovementCommand() const
{
	return {
	    .handled = true,
	    .queuedMovementCommand = true,
	};
}

RuntimeInputRouteResult RuntimeInputRouteResultBuilder::blockedMovement(PlayerActionBlockReason reason) const
{
	return {
	    .movementBlockReason = reason,
	};
}

} // namespace dev
