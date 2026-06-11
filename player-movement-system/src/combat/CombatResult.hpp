#pragma once

namespace dev {

enum class CombatResultType {
	Hit,
	Defeated,
	InvalidTarget,
};

struct CombatResult {
	CombatResultType type = CombatResultType::InvalidTarget;
	int damage = 0;
	int remainingHitPoints = 0;
};

} // namespace dev

