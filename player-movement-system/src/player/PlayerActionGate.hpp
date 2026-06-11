#pragma once

#include "focus/InputFocus.hpp"
#include "player/Player.hpp"

namespace dev {

struct PlayerActionContext {
	bool paused = false;
	bool animationLocked = false;
};

enum class PlayerActionBlockReason {
	None,
	Focus,
	Paused,
	AnimationLocked,
	AnimationCommitment,
	Stunned,
};

class PlayerActionGate {
public:
	PlayerActionGate(const InputFocus &focus, const PlayerActionContext &context);

	[[nodiscard]] PlayerActionBlockReason movementBlockReason(const Player &player) const;
	bool canMove(const Player &player) const;

private:
	const InputFocus &focus_;
	const PlayerActionContext &context_;
};

} // namespace dev
