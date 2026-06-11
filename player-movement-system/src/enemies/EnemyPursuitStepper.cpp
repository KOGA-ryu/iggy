#include "EnemyPursuitStepper.hpp"

namespace dev {

EnemyPursuitStepper::EnemyPursuitStepper(const TileMap &map, const Collision &collision, const EnemyAttackRunner &attacks)
    : map_(map)
    , collision_(collision)
    , attacks_(attacks)
{
}

void EnemyPursuitStepper::pursue(Enemy &enemy, Player &target) const
{
	enemy.moveState = EnemyMoveState::Pursuing;
	for (int step = 0; step < enemy.tuning.maxStepsPerTick; ++step) {
		const Point next = stepPlanner_.nextStepToward(enemy.position.future, target.position.tile);
		if (next == enemy.position.future)
			return;
		if (!map_.isWalkable(next) || collision_.blocksMovement(next))
			return;
		stepCommitter_.commit(enemy.position, next);
		if (attacks_.targetInAttackRange(enemy, target))
			return;
	}
}

} // namespace dev
