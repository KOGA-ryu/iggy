#pragma once

namespace dev {

enum class EnemyAttackTransition {
	None,
	WindupStarted,
	WindupCompleted,
	RecoveryCompleted,
	RecoveryCompletedAndWindupStarted,
};

struct EnemyAttackResult {
	bool consumedFrame = false;
	EnemyAttackTransition transition = EnemyAttackTransition::None;
};

} // namespace dev
