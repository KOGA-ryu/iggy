#pragma once

#include "interaction/DestinationAction.hpp"
#include "player/Player.hpp"

namespace dev {

class ActionRules {
public:
	bool canExecute(const Player &player, const DestinationAction &action) const;
	bool targetInRange(const Player &player, const DestinationAction &action) const;
	bool targetStillValid(const DestinationAction &action) const;
};

} // namespace dev

