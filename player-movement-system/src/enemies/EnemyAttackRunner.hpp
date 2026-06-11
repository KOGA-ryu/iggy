#pragma once

#include "combat/CombatSystem.hpp"
#include "enemies/Enemy.hpp"
#include "enemies/EnemyAttackEntryPolicy.hpp"
#include "enemies/EnemyAttackPhaseRunner.hpp"
#include "enemies/EnemyAttackRestartPolicy.hpp"
#include "enemies/EnemyAttackResult.hpp"
#include "player/Player.hpp"

namespace dev {

class EnemyAttackRunner {
public:
	explicit EnemyAttackRunner(CombatSystem *combatSystem = nullptr);

	EnemyAttackResult update(Enemy &enemy, Player &target, float deltaSeconds) const;
	[[nodiscard]] bool targetInAttackRange(const Enemy &enemy, const Player &target) const;

private:
	CombatSystem *combatSystem_;
	EnemyAttackEntryPolicy entryPolicy_;
	EnemyAttackPhaseRunner phaseRunner_;
	EnemyAttackRestartPolicy restartPolicy_;
};

} // namespace dev
