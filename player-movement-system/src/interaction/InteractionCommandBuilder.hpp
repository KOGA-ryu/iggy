#pragma once

#include "commands/MovementCommand.hpp"
#include "interaction/InteractionIntent.hpp"
#include "player/PlayerActionGate.hpp"

namespace dev {

class InteractionCommandBuilder {
public:
	MovementCommand build(
	    PlayerId playerId,
	    const Player &player,
	    const InteractionIntent &intent,
	    const PlayerActionGate &gate) const;

private:
	DestinationAction destinationActionFor(const InteractionIntent &intent) const;
};

} // namespace dev

