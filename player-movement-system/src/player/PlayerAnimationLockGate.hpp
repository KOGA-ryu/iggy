#pragma once

#include "events/MovementEventSink.hpp"
#include "player/Player.hpp"

namespace dev {

class PlayerAnimationLockGate {
public:
	explicit PlayerAnimationLockGate(MovementEventSink *eventSink = nullptr);

	bool advance(Player &player, float deltaSeconds) const;

private:
	MovementEventSink *eventSink_;
};

} // namespace dev
