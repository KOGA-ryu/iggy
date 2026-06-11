#pragma once

#include "enemies/EnemyState.hpp"
#include "enemies/EnemyTuning.hpp"
#include "player/ActorPosition.hpp"

namespace dev {

struct Enemy {
	ActorPosition position;
	EnemyMoveState moveState = EnemyMoveState::Idle;
	EnemyTuning tuning;
	float stateTimerSeconds = 0.0F;
};

} // namespace dev

