#pragma once

namespace dev {

enum class EnemyPursuitStopReason {
	BudgetSpent,
	Blocked,
	AlreadyAtTarget,
	AttackRangeReached,
};

struct EnemyPursuitResult {
	int stepsCommitted = 0;
	EnemyPursuitStopReason stopReason = EnemyPursuitStopReason::BudgetSpent;
};

} // namespace dev
