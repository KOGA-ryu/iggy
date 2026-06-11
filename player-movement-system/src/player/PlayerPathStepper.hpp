#pragma once

#include "events/MovementEventSink.hpp"
#include "player/Player.hpp"
#include "world/Collision.hpp"

namespace dev {

struct PlayerPathStepResult {
	bool consumedStep = false;
	bool actionReady = false;
};

class PlayerPathStepper {
public:
	PlayerPathStepper(const Collision &collision, MovementEventSink *eventSink = nullptr);

	PlayerPathStepResult step(Player &player) const;

private:
	const Collision &collision_;
	MovementEventSink *eventSink_;
};

} // namespace dev
