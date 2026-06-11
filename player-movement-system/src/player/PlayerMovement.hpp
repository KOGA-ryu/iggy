#pragma once

#include <vector>

#include "actions/ActionExecutor.hpp"
#include "player/Player.hpp"
#include "world/Collision.hpp"

namespace dev {

class PlayerMovement {
public:
	PlayerMovement(const Collision &collision, ActionExecutor actionExecutor = ActionExecutor {});

	void update(std::vector<Player> &players, float deltaSeconds) const;

private:
	const Collision &collision_;
	ActionExecutor actionExecutor_;
};

} // namespace dev
