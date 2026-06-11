#include "CombatResolver.hpp"

#include <algorithm>

namespace dev {

CombatResult CombatResolver::resolveAttack(const CombatStats &attacker, Combatant &target) const
{
	if (!attacker.alive() || !target.stats.alive())
		return { CombatResultType::InvalidTarget, 0, target.stats.hitPoints };

	const int damage = std::max(1, attacker.attackPower - target.stats.defense);
	target.stats.hitPoints = std::max(0, target.stats.hitPoints - damage);

	return {
		.type = target.stats.alive() ? CombatResultType::Hit : CombatResultType::Defeated,
		.damage = damage,
		.remainingHitPoints = target.stats.hitPoints,
	};
}

} // namespace dev

