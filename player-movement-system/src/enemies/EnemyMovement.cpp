#include "EnemyMovement.hpp"

#include <algorithm>
#include <cstdlib>

namespace dev {

namespace {

int Sign(int value)
{
	if (value == 0)
		return 0;
	return value > 0 ? 1 : -1;
}

int ChebyshevDistance(Point a, Point b)
{
	return std::max(std::abs(a.x - b.x), std::abs(a.y - b.y));
}

} // namespace

EnemyMovement::EnemyMovement(const TileMap &map, const Collision &collision, MovementEventSink *eventSink)
    : map_(map)
    , collision_(collision)
    , eventSink_(eventSink)
{
}

void EnemyMovement::update(std::vector<Enemy> &enemies, const Player &target, float deltaSeconds) const
{
	for (Enemy &enemy : enemies) {
		updateEnemy(enemy, target, deltaSeconds);
	}
}

void EnemyMovement::updateEnemy(Enemy &enemy, const Player &target, float deltaSeconds) const
{
	if (enemy.moveState == EnemyMoveState::Attacking) {
		enemy.stateTimerSeconds += deltaSeconds;
		if (enemy.stateTimerSeconds >= enemy.tuning.attackWindupSeconds) {
			enemy.moveState = EnemyMoveState::Recovering;
			enemy.stateTimerSeconds = 0.0F;
		}
		return;
	}

	if (enemy.moveState == EnemyMoveState::Recovering) {
		enemy.stateTimerSeconds += deltaSeconds;
		if (enemy.stateTimerSeconds < enemy.tuning.attackRecoverySeconds)
			return;
		enemy.stateTimerSeconds = 0.0F;
	}

	if (targetInAttackRange(enemy, target)) {
		enemy.moveState = EnemyMoveState::Attacking;
		enemy.stateTimerSeconds = 0.0F;
		return;
	}

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
		if (targetInAttackRange(enemy, target))
			return;
	}
}

bool EnemyMovement::targetInAttackRange(const Enemy &enemy, const Player &target) const
{
	return ChebyshevDistance(enemy.position.tile, target.position.tile) <= enemy.tuning.attackRangeTiles;
}

Point EnemyMovement::nextStepToward(Point from, Point to) const
{
	return {
		from.x + Sign(to.x - from.x),
		from.y + Sign(to.y - from.y),
	};
}

} // namespace dev

