#pragma once

#include <vector>

#include "scene/level/LevelTileRenderChunkCache.hpp"
#include "scene/level/TileCoord.hpp"

namespace iggy {

enum class LevelTileRenderDirtyChunksIssueCode {
	InvalidChunkSize,
};

struct LevelTileRenderDirtyChunksIssue {
	LevelTileRenderDirtyChunksIssueCode code = LevelTileRenderDirtyChunksIssueCode::InvalidChunkSize;
	int chunkWidth = 0;
	int chunkHeight = 0;
};

struct LevelTileRenderDirtyChunksConfig {
	int chunkWidth = 1;
	int chunkHeight = 1;
};

struct LevelTileRenderDirtyChunksResult {
	bool queried = false;
	std::vector<LevelTileRenderChunkCoord> chunks;
	std::vector<LevelTileRenderDirtyChunksIssue> issues;
};

class LevelTileRenderDirtyChunks {
public:
	[[nodiscard]] LevelTileRenderDirtyChunksResult query(const std::vector<TileCoord> &changedTiles, const LevelTileRenderDirtyChunksConfig &config) const;
	[[nodiscard]] LevelTileRenderDirtyChunksResult query(const std::vector<TileCoord> &changedTiles, const LevelTileRenderChunkCacheConfig &config) const;
};

} // namespace iggy
