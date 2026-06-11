#pragma once

#include "focus/InputFocus.hpp"
#include "player/Player.hpp"

namespace dev {

struct PlayerActionContext {
	bool paused = false;
	bool animationLocked = false;
};

class PlayerActionGate {
public:
	PlayerActionGate(const InputFocus &focus, const PlayerActionContext &context);

	bool canMove(const Player &player) const;

private:
	const InputFocus &focus_;
	const PlayerActionContext &context_;
};

} // namespace dev

