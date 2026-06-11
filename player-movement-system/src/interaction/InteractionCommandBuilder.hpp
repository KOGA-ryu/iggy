#pragma once

#include "commands/MovementCommand.hpp"
#include "interaction/DestinationActionBuilder.hpp"
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
	DestinationActionBuilder actions_;
};

} // namespace dev
