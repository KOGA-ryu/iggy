#include "EnemyPursuitStepper.hpp"

#include <cstdlib>

namespace dev {

namespace {

int Sign(int value)
{
	if (value == 0)
		return 0;
	return value > 0 ? 1 : -1;
}

} // namespace

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
		const Point next = nextStepToward(enemy.position.future, target.position.tile);
		if (next == enemy.position.future)
			return;
		if (!map_.isWalkable(next) || collision_.blocksMovement(next))
			return;
		enemy.position.previous = enemy.position.tile;
		enemy.position.future = next;
		enemy.position.tile = next;
		enemy.position.precise = next;
		if (attacks_.targetInAttackRange(enemy, target))
			return;
	}
}

Point EnemyPursuitStepper::nextStepToward(Point from, Point to) const
{
	return {
	    from.x + Sign(to.x - from.x),
	    from.y + Sign(to.y - from.y),
	};
}

} // namespace dev
