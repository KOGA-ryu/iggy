#pragma once

#include "app/RuntimeInputTypes.hpp"
#include "app/RuntimeTargetInteractionInputStep.hpp"
#include "commands/MovementCommandSource.hpp"
#include "input/RawInput.hpp"
#include "player/PlayerActionGate.hpp"

namespace dev {

class RuntimeTargetInputRouter {
public:
	explicit RuntimeTargetInputRouter(QueuedMovementCommandSource &movementCommands);

	[[nodiscard]] RuntimeInputRouteResult route(
	    const RawInputEvent &event,
	    const RuntimeInputContext &context,
	    const Player &player,
	    const PlayerActionGate &gate) const;

private:
	RuntimeTargetInteractionInputStep targetInteractionInput_;
};

} // namespace dev
