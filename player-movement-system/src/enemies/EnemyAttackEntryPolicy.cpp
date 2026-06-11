#include "EnemyAttackEntryPolicy.hpp"

namespace dev {

bool EnemyAttackEntryPolicy::shouldStartWindup(const Enemy &enemy, const Player &target) const
{
	return attackRange_.contains(enemy, target);
}

} // namespace dev
