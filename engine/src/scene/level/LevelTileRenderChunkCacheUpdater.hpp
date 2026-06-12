#pragma once

#include <vector>

#include "scene/level/LevelTileMap.hpp"
#include "scene/level/LevelTileRenderChunkCache.hpp"

namespace iggy {

enum class LevelTileRenderChunkCacheUpdateIssueCode {
	InvalidChunkSize,
};

struct LevelTileRenderChunkCacheUpdateIssue {
	LevelTileRenderChunkCacheUpdateIssueCode code = LevelTileRenderChunkCacheUpdateIssueCode::InvalidChunkSize;
	int chunkWidth = 0;
	int chunkHeight = 0;
};

struct LevelTileRenderChunkCacheUpdateResult {
	bool updated = false;
	LevelTileRenderChunkCache cache;
	std::vector<LevelTileRenderChunkCoord> rebuiltChunks;
	std::vector<LevelTileRenderChunkCoord> skippedChunks;
	std::vector<LevelTileRenderChunkCacheUpdateIssue> issues;
};

class LevelTileRenderChunkCacheUpdater {
public:
	[[nodiscard]] LevelTileRenderChunkCacheUpdateResult update(
		const LevelTileRenderChunkCache &current,
		const LevelTileMap &map,
		const LevelTileRenderChunkCacheConfig &config,
		const std::vector<LevelTileRenderChunkCoord> &dirtyChunks) const;
};

} // namespace iggy
