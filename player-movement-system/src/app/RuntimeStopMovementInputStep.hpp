#pragma once

#include "app/RuntimeInputTypes.hpp"
#include "app/RuntimeMovementCommandQueueStep.hpp"
#include "commands/IntentCommandBuilder.hpp"
#include "input/RawInput.hpp"
#include "player/Player.hpp"
#include "player/PlayerActionGate.hpp"

namespace dev {

class RuntimeStopMovementInputStep {
public:
	explicit RuntimeStopMovementInputStep(QueuedMovementCommandSource &movementCommands);

	[[nodiscard]] RuntimeInputRouteResult route(
	    const RawInputEvent &event,
	    int stopKey,
	    PlayerId playerId,
	    const Player &player,
	    const PlayerActionGate &gate,
	    PlayerActionBlockReason blockReason) const;

private:
	RuntimeMovementCommandQueueStep queueStep_;
	IntentCommandBuilder commandBuilder_;
};

} // namespace dev
