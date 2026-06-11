#pragma once

#include "app/RuntimeInputTypes.hpp"
#include "app/RuntimeMovementCommandQueueStep.hpp"
#include "input/RawInput.hpp"
#include "interaction/InteractionCommandBuilder.hpp"
#include "interaction/InteractionIntentBuilder.hpp"
#include "player/Player.hpp"
#include "player/PlayerActionGate.hpp"
#include "targeting/TargetResolver.hpp"
#include "world/TileMap.hpp"

namespace dev {

class RuntimeTargetInteractionInputStep {
public:
	explicit RuntimeTargetInteractionInputStep(QueuedMovementCommandSource &movementCommands);

	[[nodiscard]] RuntimeInputRouteResult route(
	    const RawInputEvent &event,
	    const TileMap &map,
	    const TargetResolver &targetResolver,
	    PlayerId playerId,
	    const Player &player,
	    const PlayerActionGate &gate) const;

private:
	RuntimeMovementCommandQueueStep queueStep_;
	InteractionIntentBuilder interactionIntentBuilder_;
	InteractionCommandBuilder interactionCommandBuilder_;
};

} // namespace dev
