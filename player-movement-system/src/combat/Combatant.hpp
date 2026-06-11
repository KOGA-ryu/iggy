#pragma once

#include "combat/CombatStats.hpp"
#include "targeting/Target.hpp"

namespace dev {

struct Combatant {
	Target target;
	CombatStats stats;
};

} // namespace dev

