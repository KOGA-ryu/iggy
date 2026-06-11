#include "EnemyPursuitStepper.hpp"

namespace dev {

EnemyPursuitStepper::EnemyPursuitStepper(const TileMap &map, const Collision &collision)
    : stepGate_(map, collision)
{
}

EnemyPursuitResult EnemyPursuitStepper::pursue(Enemy &enemy, Player &target) const
{
	enemy.moveState = EnemyMoveState::Pursuing;
	EnemyPursuitResult result;
	for (int stepsSpent = 0; budget_.canSpendStep(enemy, stepsSpent); ++stepsSpent) {
		const Point next = stepPlanner_.nextStepToward(enemy.position.future, target.position.tile);
		if (next == enemy.position.future) {
			result.stopReason = EnemyPursuitStopReason::AlreadyAtTarget;
			return result;
		}
		if (!stepGate_.canEnter(next)) {
			result.stopReason = EnemyPursuitStopReason::Blocked;
			return result;
		}
		stepCommitter_.commit(enemy.position, next);
		++result.stepsCommitted;
		if (attackRange_.contains(enemy, target)) {
			result.stopReason = EnemyPursuitStopReason::AttackRangeReached;
			return result;
		}
	}
	return result;
}

} // namespace dev
