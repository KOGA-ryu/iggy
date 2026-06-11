#pragma once

#include "enemies/Enemy.hpp"

namespace dev {

class EnemyPursuitBudget {
public:
	[[nodiscard]] bool canSpendStep(const Enemy &enemy, int stepsSpent) const;
};

} // namespace dev
