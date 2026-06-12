#pragma once

#include <vector>

#include "scene/level/LevelTileMap.hpp"
#include "scene/level/LevelTileRenderChunkCache.hpp"
#include "scene/level/LevelTileRenderChunkCacheUpdater.hpp"
#include "scene/level/LevelTileRenderDirtyChunks.hpp"
#include "scene/level/TileCoord.hpp"

namespace iggy {

struct LevelRenderCacheState {
	LevelTileRenderChunkCacheConfig tileChunkConfig;
	LevelTileRenderChunkCache tileChunks;
};

struct LevelRenderCacheBuildResult {
	bool built = false;
	LevelRenderCacheState state;
	std::vector<LevelTileRenderChunkCacheIssue> tileChunkIssues;
};

class LevelRenderCacheBuilder {
public:
	[[nodiscard]] LevelRenderCacheBuildResult build(const LevelTileMap &map, const LevelTileRenderChunkCacheConfig &config) const;
};

struct LevelRenderCacheUpdateResult {
	bool updated = false;
	LevelRenderCacheState state;
	LevelTileRenderDirtyChunksResult dirtyChunks;
	LevelTileRenderChunkCacheUpdateResult tileChunkUpdate;
};

class LevelRenderCacheUpdater {
public:
	[[nodiscard]] LevelRenderCacheUpdateResult update(const LevelRenderCacheState &current, const LevelTileMap &map, const std::vector<TileCoord> &changedTiles) const;
};

} // namespace iggy
