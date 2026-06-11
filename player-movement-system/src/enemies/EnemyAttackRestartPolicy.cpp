#include "EnemyAttackRestartPolicy.hpp"

namespace dev {

bool EnemyAttackRestartPolicy::shouldRestartAfterRecovery(const Enemy &enemy, const Player &target) const
{
	return attackRange_.contains(enemy, target);
}

} // namespace dev
