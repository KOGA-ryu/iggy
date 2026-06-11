#pragma once

#include "combat/CombatSystem.hpp"
#include "enemies/Enemy.hpp"
#include "enemies/EnemyAttackRange.hpp"
#include "player/Player.hpp"

namespace dev {

class EnemyAttackRunner {
public:
	explicit EnemyAttackRunner(CombatSystem *combatSystem = nullptr);

	bool update(Enemy &enemy, Player &target, float deltaSeconds) const;
	[[nodiscard]] bool targetInAttackRange(const Enemy &enemy, const Player &target) const;

private:
	CombatSystem *combatSystem_;
	EnemyAttackRange attackRange_;
};

} // namespace dev
