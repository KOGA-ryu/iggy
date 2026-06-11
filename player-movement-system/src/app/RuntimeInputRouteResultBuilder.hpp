#pragma once

#include "app/RuntimeInputTypes.hpp"

namespace dev {

class RuntimeInputRouteResultBuilder {
public:
	[[nodiscard]] RuntimeInputRouteResult unhandled() const;
	[[nodiscard]] RuntimeInputRouteResult queuedSessionCommand() const;
	[[nodiscard]] RuntimeInputRouteResult queuedMovementCommand() const;
	[[nodiscard]] RuntimeInputRouteResult blockedMovement(PlayerActionBlockReason reason) const;
};

} // namespace dev
