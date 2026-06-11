#include "WorldEntityService.hpp"

#include <algorithm>

namespace dev {

Enemy &WorldEntityService::spawnEnemy(SimulationWorld &world, const EnemySpawnRequest &request) const
{
	(void)despawnEnemy(world, request.id);

	Enemy enemy;
	enemy.id = request.id;
	enemy.position.tile = request.tile;
	enemy.position.future = request.tile;
	enemy.position.previous = request.tile;
	enemy.position.precise = request.tile;
	enemy.tuning = request.tuning;
	enemy.combatStats = request.combatStats;

	world.enemies.push_back(enemy);

	const Target target = enemyTarget(request.id, request.tile);
	world.combat.registry().add({
	    .target = target,
	    .stats = request.combatStats,
	});
	world.targets.add(target);

	return world.enemies.back();
}

bool WorldEntityService::despawnEnemy(SimulationWorld &world, TargetId id) const
{
	bool removed = false;

	const auto before = world.enemies.size();
	world.enemies.erase(
	    std::remove_if(world.enemies.begin(), world.enemies.end(), [id](const Enemy &enemy) {
		    return enemy.id == id;
	    }),
	    world.enemies.end());
	removed = world.enemies.size() != before;

	const Target target = enemyTarget(id, {});
	removed = world.combat.registry().remove(target) || removed;
	removed = world.targets.remove(TargetType::Enemy, id) || removed;

	return removed;
}

Item &WorldEntityService::spawnItem(SimulationWorld &world, const ItemSpawnRequest &request) const
{
	(void)despawnItem(world, request.id);

	Item item {
		.id = request.id,
		.tile = request.tile,
	};
	world.items.push_back(item);
	world.targets.add(itemTarget(request.id, request.tile));
	return world.items.back();
}

bool WorldEntityService::despawnItem(SimulationWorld &world, TargetId id) const
{
	const auto before = world.items.size();
	world.items.erase(
	    std::remove_if(world.items.begin(), world.items.end(), [id](const Item &item) {
		    return item.id == id;
	    }),
	    world.items.end());

	bool removed = world.items.size() != before;
	removed = world.targets.remove(TargetType::Item, id) || removed;
	return removed;
}

Target WorldEntityService::enemyTarget(TargetId id, Point tile) const
{
	return {
		.type = TargetType::Enemy,
		.id = id,
		.tile = tile,
	};
}

Target WorldEntityService::itemTarget(TargetId id, Point tile) const
{
	return {
		.type = TargetType::Item,
		.id = id,
		.tile = tile,
	};
}

} // namespace dev
