#pragma once

#include "app/RuntimeInputTypes.hpp"
#include "commands/MovementCommandSource.hpp"
#include "input/RawInput.hpp"
#include "interaction/InteractionCommandBuilder.hpp"
#include "interaction/InteractionIntentBuilder.hpp"
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
	QueuedMovementCommandSource &movementCommands_;
	InteractionIntentBuilder interactionIntentBuilder_;
	InteractionCommandBuilder interactionCommandBuilder_;
};

} // namespace dev
