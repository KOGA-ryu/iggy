#pragma once

namespace dev {

enum class PlayerMoveState {
	Idle,
	Pathing,
	Stepping,
	Blocked,
	Stunned,
	Acting,
};

} // namespace dev
