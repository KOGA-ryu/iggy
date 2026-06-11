#include "EnemyAttackPhaseRunner.hpp"

namespace dev {

EnemyAttackResult EnemyAttackPhaseRunner::advanceWindup(Enemy &enemy, Player &target, float deltaSeconds, CombatSystem *combatSystem) const
{
	enemy.stateTimerSeconds += deltaSeconds;
	if (enemy.stateTimerSeconds < enemy.tuning.attackWindupSeconds)
		return { .consumedFrame = true };

	if (combatSystem != nullptr)
		combatSystem->resolveEnemyAttack(enemy, target);
	enemy.moveState = EnemyMoveState::Recovering;
	enemy.stateTimerSeconds = 0.0F;
	return { .consumedFrame = true, .transition = EnemyAttackTransition::WindupCompleted };
}

EnemyAttackResult EnemyAttackPhaseRunner::advanceRecovery(Enemy &enemy, float deltaSeconds) const
{
	enemy.stateTimerSeconds += deltaSeconds;
	if (enemy.stateTimerSeconds < enemy.tuning.attackRecoverySeconds)
		return { .consumedFrame = true };

	enemy.stateTimerSeconds = 0.0F;
	return { .transition = EnemyAttackTransition::RecoveryCompleted };
}

void EnemyAttackPhaseRunner::startWindup(Enemy &enemy) const
{
	enemy.moveState = EnemyMoveState::Attacking;
	enemy.stateTimerSeconds = 0.0F;
}

} // namespace dev
