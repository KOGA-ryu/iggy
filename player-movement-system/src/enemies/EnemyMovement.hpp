#pragma once

#include <vector>

#include "combat/CombatSystem.hpp"
#include "enemies/Enemy.hpp"
#include "enemies/EnemyAttackRunner.hpp"
#include "enemies/EnemyPursuitStepper.hpp"
#include "events/MovementEventSink.hpp"
#include "player/Player.hpp"
#include "world/Collision.hpp"
#include "world/TileMap.hpp"

namespace dev {

class EnemyMovement {
public:
	EnemyMovement(const TileMap &map, const Collision &collision, MovementEventSink *eventSink = nullptr, CombatSystem *combatSystem = nullptr);

	void update(std::vector<Enemy> &enemies, Player &target, float deltaSeconds) const;

private:
	void updateEnemy(Enemy &enemy, Player &target, float deltaSeconds) const;

	const TileMap &map_;
	const Collision &collision_;
	EnemyAttackRunner attacks_;
	EnemyPursuitStepper pursuit_;
};

} // namespace dev
