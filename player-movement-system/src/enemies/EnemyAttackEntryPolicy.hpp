#pragma once

#include "enemies/Enemy.hpp"
#include "enemies/EnemyAttackRange.hpp"
#include "player/Player.hpp"

namespace dev {

class EnemyAttackEntryPolicy {
public:
	[[nodiscard]] bool shouldStartWindup(const Enemy &enemy, const Player &target) const;

private:
	EnemyAttackRange attackRange_;
};

} // namespace dev
