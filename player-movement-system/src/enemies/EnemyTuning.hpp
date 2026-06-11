#pragma once

namespace dev {

struct EnemyTuning {
	int maxStepsPerTick = 1;
	int attackRangeTiles = 1;
	float attackWindupSeconds = 0.35F;
	float attackRecoverySeconds = 0.45F;
};

} // namespace dev

