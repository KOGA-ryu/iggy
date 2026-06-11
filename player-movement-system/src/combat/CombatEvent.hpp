#pragma once

#include "combat/CombatResult.hpp"
#include "targeting/Target.hpp"

namespace dev {

enum class CombatEventType {
	Hit,
	Defeated,
	Rejected,
};

struct CombatEvent {
	CombatEventType type = CombatEventType::Rejected;
	Target target;
	int damage = 0;
	int remainingHitPoints = 0;
	CombatResultType result = CombatResultType::InvalidTarget;
};

} // namespace dev

