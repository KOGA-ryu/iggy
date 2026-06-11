#include "EnemyPursuitStepper.hpp"

namespace dev {

EnemyPursuitStepper::EnemyPursuitStepper(const TileMap &map, const Collision &collision)
    : stepGate_(map, collision)
{
}

void EnemyPursuitStepper::pursue(Enemy &enemy, Player &target) const
{
	enemy.moveState = EnemyMoveState::Pursuing;
	for (int stepsSpent = 0; budget_.canSpendStep(enemy, stepsSpent); ++stepsSpent) {
		const Point next = stepPlanner_.nextStepToward(enemy.position.future, target.position.tile);
		if (next == enemy.position.future)
			return;
		if (!stepGate_.canEnter(next))
			return;
		stepCommitter_.commit(enemy.position, next);
		if (attackRange_.contains(enemy, target))
			return;
	}
}

} // namespace dev
