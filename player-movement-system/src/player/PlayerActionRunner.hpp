#pragma once

#include "actions/ActionExecutor.hpp"
#include "player/Player.hpp"

namespace dev {

class PlayerActionRunner {
public:
	explicit PlayerActionRunner(ActionExecutor actionExecutor = ActionExecutor {});

	void run(Player &player, PlayerId playerId = 0) const;

private:
	ActionExecutor actionExecutor_;
};

} // namespace dev
