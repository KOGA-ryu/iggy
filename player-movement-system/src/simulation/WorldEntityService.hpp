#pragma once

#include "combat/CombatStats.hpp"
#include "enemies/Enemy.hpp"
#include "enemies/EnemyTuning.hpp"
#include "items/Item.hpp"
#include "simulation/SimulationWorld.hpp"
#include "targeting/Target.hpp"
#include "world/Point.hpp"

namespace dev {

struct EnemySpawnRequest {
	TargetId id = 0;
	Point tile;
	EnemyTuning tuning;
	CombatStats combatStats;
};

struct ItemSpawnRequest {
	TargetId id = 0;
	Point tile;
};

class WorldEntityService {
public:
	Enemy &spawnEnemy(SimulationWorld &world, const EnemySpawnRequest &request) const;
	[[nodiscard]] bool despawnEnemy(SimulationWorld &world, TargetId id) const;
	Item &spawnItem(SimulationWorld &world, const ItemSpawnRequest &request) const;
	[[nodiscard]] bool despawnItem(SimulationWorld &world, TargetId id) const;

private:
	[[nodiscard]] Target enemyTarget(TargetId id, Point tile) const;
	[[nodiscard]] Target itemTarget(TargetId id, Point tile) const;
};

} // namespace dev
