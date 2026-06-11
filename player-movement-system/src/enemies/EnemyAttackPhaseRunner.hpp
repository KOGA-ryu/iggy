#pragma once

#include "combat/CombatSystem.hpp"
#include "enemies/Enemy.hpp"
#include "enemies/EnemyAttackResult.hpp"
#include "player/Player.hpp"

namespace dev {

class EnemyAttackPhaseRunner {
public:
	EnemyAttackResult advanceWindup(Enemy &enemy, Player &target, float deltaSeconds, CombatSystem *combatSystem) const;
	EnemyAttackResult advanceRecovery(Enemy &enemy, float deltaSeconds) const;
	void startWindup(Enemy &enemy) const;
};

} // namespace dev
