#pragma once

#include "enemies/Enemy.hpp"
#include "enemies/EnemyAttackRunner.hpp"
#include "player/Player.hpp"
#include "world/Collision.hpp"
#include "world/TileMap.hpp"

namespace dev {

class EnemyPursuitStepper {
public:
	EnemyPursuitStepper(const TileMap &map, const Collision &collision, const EnemyAttackRunner &attacks);

	void pursue(Enemy &enemy, Player &target) const;

private:
	[[nodiscard]] Point nextStepToward(Point from, Point to) const;

	const TileMap &map_;
	const Collision &collision_;
	const EnemyAttackRunner &attacks_;
};

} // namespace dev
