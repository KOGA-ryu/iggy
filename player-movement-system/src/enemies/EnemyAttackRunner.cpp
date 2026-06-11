#include "EnemyAttackRunner.hpp"

namespace dev {

EnemyAttackRunner::EnemyAttackRunner(CombatSystem *combatSystem)
    : combatSystem_(combatSystem)
{
}

bool EnemyAttackRunner::update(Enemy &enemy, Player &target, float deltaSeconds) const
{
	if (enemy.moveState == EnemyMoveState::Attacking) {
		enemy.stateTimerSeconds += deltaSeconds;
		if (enemy.stateTimerSeconds >= enemy.tuning.attackWindupSeconds) {
			if (combatSystem_ != nullptr)
				combatSystem_->resolveEnemyAttack(enemy, target);
			enemy.moveState = EnemyMoveState::Recovering;
			enemy.stateTimerSeconds = 0.0F;
		}
		return true;
	}

	if (enemy.moveState == EnemyMoveState::Recovering) {
		enemy.stateTimerSeconds += deltaSeconds;
		if (enemy.stateTimerSeconds < enemy.tuning.attackRecoverySeconds)
			return true;
		enemy.stateTimerSeconds = 0.0F;
	}

	if (!targetInAttackRange(enemy, target))
		return false;

	enemy.moveState = EnemyMoveState::Attacking;
	enemy.stateTimerSeconds = 0.0F;
	return true;
}

bool EnemyAttackRunner::targetInAttackRange(const Enemy &enemy, const Player &target) const
{
	return attackRange_.contains(enemy, target);
}

} // namespace dev
