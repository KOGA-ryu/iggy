#include "EnemyAttackRange.hpp"

#include <algorithm>
#include <cstdlib>

namespace dev {

namespace {

int ChebyshevDistance(Point a, Point b)
{
	return std::max(std::abs(a.x - b.x), std::abs(a.y - b.y));
}

} // namespace

bool EnemyAttackRange::contains(const Enemy &enemy, const Player &target) const
{
	return ChebyshevDistance(enemy.position.tile, target.position.tile) <= enemy.tuning.attackRangeTiles;
}

} // namespace dev
