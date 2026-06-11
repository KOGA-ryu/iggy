#pragma once

#include <optional>

#include "commands/MovementCommand.hpp"
#include "input/PlayerIntent.hpp"
#include "player/PlayerActionGate.hpp"

namespace dev {

class IntentCommandBuilder {
public:
	std::optional<MovementCommand> buildMoveCommand(
	    PlayerId playerId,
	    const Player &player,
	    const PlayerIntent &intent,
	    const PlayerActionGate &gate) const;
};

} // namespace dev

