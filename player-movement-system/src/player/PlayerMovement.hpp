#pragma once

#include <vector>

#include "actions/ActionExecutor.hpp"
#include "events/MovementEventSink.hpp"
#include "player/Player.hpp"
#include "player/PlayerAnimationLockGate.hpp"
#include "player/PlayerPathStepper.hpp"
#include "world/Collision.hpp"

namespace dev {

class PlayerMovement {
public:
	PlayerMovement(const Collision &collision, ActionExecutor actionExecutor = ActionExecutor {}, MovementEventSink *eventSink = nullptr);

	void update(std::vector<Player> &players, float deltaSeconds) const;

private:
	PlayerAnimationLockGate animationLocks_;
	PlayerPathStepper pathStepper_;
	ActionExecutor actionExecutor_;
};

} // namespace dev
