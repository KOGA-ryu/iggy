#pragma once

#include "app/RuntimeMovementCommandQueueStep.hpp"
#include "app/RuntimeInputTypes.hpp"
#include "commands/IntentCommandBuilder.hpp"
#include "input/InputMapper.hpp"
#include "input/RawInput.hpp"
#include "player/Player.hpp"
#include "player/PlayerActionGate.hpp"
#include "world/TileMap.hpp"

namespace dev {

class RuntimeMovementIntentInputStep {
public:
	explicit RuntimeMovementIntentInputStep(QueuedMovementCommandSource &movementCommands);

	[[nodiscard]] RuntimeInputRouteResult route(
	    const RawInputEvent &event,
	    const TileMap &map,
	    PlayerId playerId,
	    const Player &player,
	    const InputFocus &focus,
	    const PlayerActionGate &gate) const;

private:
	RuntimeMovementCommandQueueStep queueStep_;
	InputMapper inputMapper_;
	IntentCommandBuilder commandBuilder_;
};

} // namespace dev
