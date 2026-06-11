#pragma once

#include "enemies/Enemy.hpp"
#include "enemies/EnemyAttackRange.hpp"
#include "enemies/EnemyPursuitBudget.hpp"
#include "enemies/EnemyPursuitStepGate.hpp"
#include "enemies/EnemyPursuitStepPlanner.hpp"
#include "player/ActorStepCommitter.hpp"
#include "player/Player.hpp"
#include "world/Collision.hpp"
#include "world/TileMap.hpp"

namespace dev {

class EnemyPursuitStepper {
public:
	EnemyPursuitStepper(const TileMap &map, const Collision &collision);

	void pursue(Enemy &enemy, Player &target) const;

private:
	EnemyAttackRange attackRange_;
	EnemyPursuitBudget budget_;
	EnemyPursuitStepPlanner stepPlanner_;
	EnemyPursuitStepGate stepGate_;
	ActorStepCommitter stepCommitter_;
};

} // namespace dev
