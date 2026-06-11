#include "EnemyAttackRunner.hpp"

#include <algorithm>
#include <cstdlib>

namespace dev {

namespace {

int ChebyshevDistance(Point a, Point b)
{
	return std::max(std::abs(a.x - b.x), std::abs(a.y - b.y));
}

} // namespace

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
	return ChebyshevDistance(enemy.position.tile, target.position.tile) <= enemy.tuning.attackRangeTiles;
}

} // namespace dev
