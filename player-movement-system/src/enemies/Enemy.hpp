#pragma once

#include "combat/CombatStats.hpp"
#include "enemies/EnemyState.hpp"
#include "enemies/EnemyTuning.hpp"
#include "player/ActorPosition.hpp"
#include "targeting/Target.hpp"

namespace dev {

struct Enemy {
	TargetId id = 0;
	ActorPosition position;
	EnemyMoveState moveState = EnemyMoveState::Idle;
	EnemyTuning tuning;
	CombatStats combatStats;
	float stateTimerSeconds = 0.0F;
};

} // namespace dev
