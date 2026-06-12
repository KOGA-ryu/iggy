#include "scene/level/LevelDerivedCacheState.hpp"

namespace iggy {

LevelDerivedCacheBuildResult LevelDerivedCacheBuilder::build(
	const LevelRuntimeState &level,
	const LevelDerivedCacheBuildConfig &config) const
{
	LevelDerivedCacheBuildResult result;

	if (config.buildRenderCache)
		result.render = LevelRenderCacheBuilder {}.build(level.map, config.renderCacheConfig);

	if (config.buildCollisionCache)
		result.collision = LevelCollisionCacheBuilder {}.build(level.map);

	if (config.buildRenderCache && !result.render.built)
		return result;

	if (config.buildCollisionCache && !result.collision.built)
		return result;

	result.built = true;
	if (config.buildRenderCache) {
		result.state.hasRenderCache = true;
		result.state.render = result.render.state;
	}
	if (config.buildCollisionCache) {
		result.state.hasCollisionCache = true;
		result.state.collision = result.collision.state;
	}
	return result;
}

LevelDerivedCacheUpdateResult LevelDerivedCacheUpdater::update(
	const LevelDerivedCacheState &current,
	const LevelRuntimeState &level,
	const std::vector<TileCoord> &changedTiles) const
{
	LevelDerivedCacheUpdateResult result;
	result.changedTiles = changedTiles;

	if (!current.hasRenderCache && !current.hasCollisionCache) {
		result.updated = true;
		result.state = current;
		return result;
	}

	LevelDerivedCacheState updatedState;

	if (current.hasRenderCache) {
		result.render = LevelRenderCacheUpdater {}.update(current.render, level.map, changedTiles);
		if (!result.render.updated)
			return result;

		updatedState.hasRenderCache = true;
		updatedState.render = result.render.state;
	}

	if (current.hasCollisionCache) {
		result.collision = LevelCollisionCacheUpdater {}.update(current.collision, level.map, changedTiles);
		if (!result.collision.updated)
			return result;

		updatedState.hasCollisionCache = true;
		updatedState.collision = result.collision.state;
	}

	result.updated = true;
	result.state = updatedState;
	return result;
}

} // namespace iggy
