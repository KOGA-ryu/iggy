#include "CombatResolver.hpp"

#include <algorithm>

namespace dev {

CombatResult CombatResolver::resolveAttack(const CombatStats &attacker, Combatant &target) const
{
	return resolveAttack(attacker, target.stats);
}

CombatResult CombatResolver::resolveAttack(const CombatStats &attacker, CombatStats &target) const
{
	if (!attacker.alive() || !target.alive())
		return { CombatResultType::InvalidTarget, 0, target.hitPoints };

	const int damage = std::max(1, attacker.attackPower - target.defense);
	target.hitPoints = std::max(0, target.hitPoints - damage);

	return {
		.type = target.alive() ? CombatResultType::Hit : CombatResultType::Defeated,
		.damage = damage,
		.remainingHitPoints = target.hitPoints,
	};
}

} // namespace dev
