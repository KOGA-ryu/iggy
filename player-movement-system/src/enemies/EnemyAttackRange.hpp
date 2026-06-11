#pragma once

#include "enemies/Enemy.hpp"
#include "player/Player.hpp"

namespace dev {

class EnemyAttackRange {
public:
	[[nodiscard]] bool contains(const Enemy &enemy, const Player &target) const;
};

} // namespace dev
