#include "EnemyAttackRunner.hpp"

namespace dev {

EnemyAttackRunner::EnemyAttackRunner(CombatSystem *combatSystem)
    : combatSystem_(combatSystem)
{
}

EnemyAttackResult EnemyAttackRunner::update(Enemy &enemy, Player &target, float deltaSeconds) const
{
	if (enemy.moveState == EnemyMoveState::Attacking) {
		return phaseRunner_.advanceWindup(enemy, target, deltaSeconds, combatSystem_);
	}

	if (enemy.moveState == EnemyMoveState::Recovering) {
		const EnemyAttackResult recovery = phaseRunner_.advanceRecovery(enemy, deltaSeconds);
		if (recovery.consumedFrame)
			return recovery;
		if (!restartPolicy_.shouldRestartAfterRecovery(enemy, target))
			return recovery;

		phaseRunner_.startWindup(enemy);
		return { .consumedFrame = true, .transition = EnemyAttackTransition::RecoveryCompletedAndWindupStarted };
	}

	if (!entryPolicy_.shouldStartWindup(enemy, target))
		return {};

	phaseRunner_.startWindup(enemy);
	return { .consumedFrame = true, .transition = EnemyAttackTransition::WindupStarted };
}

bool EnemyAttackRunner::targetInAttackRange(const Enemy &enemy, const Player &target) const
{
	return entryPolicy_.shouldStartWindup(enemy, target);
}

} // namespace dev
