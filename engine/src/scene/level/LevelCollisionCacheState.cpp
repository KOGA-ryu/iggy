#include "scene/level/LevelCollisionCacheState.hpp"

namespace iggy {

LevelCollisionCacheBuildResult LevelCollisionCacheBuilder::build(const LevelTileMap &map) const
{
	LevelCollisionCacheBuildResult result;
	result.collisionWorld = LevelCollisionWorldBuilder {}.build(map);
	if (!result.collisionWorld.built)
		return result;

	result.built = true;
	result.state.world = result.collisionWorld.world;
	return result;
}

LevelCollisionCacheUpdateResult LevelCollisionCacheUpdater::update(
	const LevelCollisionCacheState &current,
	const LevelTileMap &map,
	const std::vector<TileCoord> &changedTiles) const
{
	LevelCollisionCacheUpdateResult result;
	result.changedTiles = changedTiles;

	if (changedTiles.empty()) {
		result.updated = true;
		result.state = current;
		return result;
	}

	result.rebuild = LevelCollisionCacheBuilder {}.build(map);
	if (!result.rebuild.built)
		return result;

	result.updated = true;
	result.state = result.rebuild.state;
	return result;
}

} // namespace iggy
