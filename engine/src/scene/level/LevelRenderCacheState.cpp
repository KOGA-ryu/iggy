#include "scene/level/LevelRenderCacheState.hpp"

namespace iggy {

LevelRenderCacheBuildResult LevelRenderCacheBuilder::build(const LevelTileMap &map, const LevelTileRenderChunkCacheConfig &config) const
{
	LevelRenderCacheBuildResult result;
	result.state.tileChunkConfig = config;

	const LevelTileRenderChunkCacheBuildResult tileChunkBuild = LevelTileRenderChunkCacheBuilder {}.build(map, config);
	result.tileChunkIssues = tileChunkBuild.issues;
	if (!tileChunkBuild.built)
		return result;

	result.built = true;
	result.state.tileChunkConfig = config;
	result.state.tileChunks = tileChunkBuild.cache;
	return result;
}

LevelRenderCacheUpdateResult LevelRenderCacheUpdater::update(const LevelRenderCacheState &current, const LevelTileMap &map, const std::vector<TileCoord> &changedTiles) const
{
	LevelRenderCacheUpdateResult result;
	result.state.tileChunkConfig = current.tileChunkConfig;

	result.dirtyChunks = LevelTileRenderDirtyChunks {}.query(changedTiles, current.tileChunkConfig);
	if (!result.dirtyChunks.queried)
		return result;

	result.tileChunkUpdate = LevelTileRenderChunkCacheUpdater {}.update(current.tileChunks, map, current.tileChunkConfig, result.dirtyChunks.chunks);
	if (!result.tileChunkUpdate.updated)
		return result;

	result.updated = true;
	result.state.tileChunkConfig = current.tileChunkConfig;
	result.state.tileChunks = result.tileChunkUpdate.cache;
	return result;
}

} // namespace iggy
