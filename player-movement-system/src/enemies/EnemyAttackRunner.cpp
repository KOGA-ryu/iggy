#include "EnemyAttackRunner.hpp"

namespace dev {

EnemyAttackRunner::EnemyAttackRunner(CombatSystem *combatSystem)
    : combatSystem_(combatSystem)
{
}

EnemyAttackResult EnemyAttackRunner::update(Enemy &enemy, Player &target, float deltaSeconds) const
{
	if (enemy.moveState == EnemyMoveState::Attacking) {
		enemy.stateTimerSeconds += deltaSeconds;
		if (enemy.stateTimerSeconds >= enemy.tuning.attackWindupSeconds) {
			if (combatSystem_ != nullptr)
				combatSystem_->resolveEnemyAttack(enemy, target);
			enemy.moveState = EnemyMoveState::Recovering;
			enemy.stateTimerSeconds = 0.0F;
			return { .consumedFrame = true, .transition = EnemyAttackTransition::WindupCompleted };
		}
		return { .consumedFrame = true };
	}

	if (enemy.moveState == EnemyMoveState::Recovering) {
		enemy.stateTimerSeconds += deltaSeconds;
		if (enemy.stateTimerSeconds < enemy.tuning.attackRecoverySeconds)
			return { .consumedFrame = true };
		enemy.stateTimerSeconds = 0.0F;
		if (!targetInAttackRange(enemy, target))
			return { .transition = EnemyAttackTransition::RecoveryCompleted };

		enemy.moveState = EnemyMoveState::Attacking;
		enemy.stateTimerSeconds = 0.0F;
		return { .consumedFrame = true, .transition = EnemyAttackTransition::RecoveryCompletedAndWindupStarted };
	}

	if (!targetInAttackRange(enemy, target))
		return {};

	enemy.moveState = EnemyMoveState::Attacking;
	enemy.stateTimerSeconds = 0.0F;
	return { .consumedFrame = true, .transition = EnemyAttackTransition::WindupStarted };
}

bool EnemyAttackRunner::targetInAttackRange(const Enemy &enemy, const Player &target) const
{
	return attackRange_.contains(enemy, target);
}

} // namespace dev
