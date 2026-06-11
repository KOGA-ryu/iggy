#pragma once

#include "combat/CombatResult.hpp"
#include "combat/CombatStats.hpp"
#include "combat/Combatant.hpp"

namespace dev {

class CombatResolver {
public:
	CombatResult resolveAttack(const CombatStats &attacker, Combatant &target) const;
	CombatResult resolveAttack(const CombatStats &attacker, CombatStats &target) const;
};

} // namespace dev
