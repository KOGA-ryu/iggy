#include "EnemyPursuitBudget.hpp"

namespace dev {

bool EnemyPursuitBudget::canSpendStep(const Enemy &enemy, int stepsSpent) const
{
	return stepsSpent >= 0 && stepsSpent < enemy.tuning.maxStepsPerTick;
}

} // namespace dev
