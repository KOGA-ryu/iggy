#pragma once

#include <vector>

#include "core/math/Aabb2.hpp"
#include "scene/level/LevelTileMap.hpp"
#include "scene/level/LevelTileRenderCommands.hpp"
#include "scene/level/TileCoord.hpp"
#include "servers/render/RenderCommand2D.hpp"

namespace iggy {

struct LevelTileRenderChunkCoord {
	int x = 0;
	int y = 0;
};

enum class LevelTileRenderChunkCacheIssueCode {
	InvalidChunkSize,
};

struct LevelTileRenderChunkCacheIssue {
	LevelTileRenderChunkCacheIssueCode code = LevelTileRenderChunkCacheIssueCode::InvalidChunkSize;
	int chunkWidth = 0;
	int chunkHeight = 0;
};

struct LevelTileRenderChunk {
	LevelTileRenderChunkCoord coord;
	TileCoord minTile;
	TileCoord maxTile;
	Aabb2 worldBounds;
	render::RenderCommandList2D commands;
};

struct LevelTileRenderChunkCache {
	std::vector<LevelTileRenderChunk> chunks;
};

struct LevelTileRenderChunkCacheConfig {
	int chunkWidth = 1;
	int chunkHeight = 1;
	LevelTileRenderCommandConfig tileCommands;
};

struct LevelTileRenderChunkCacheBuildResult {
	bool built = false;
	LevelTileRenderChunkCache cache;
	std::vector<LevelTileRenderChunkCacheIssue> issues;
};

class LevelTileRenderChunkCacheBuilder {
public:
	[[nodiscard]] LevelTileRenderChunkCacheBuildResult build(const LevelTileMap &map, const LevelTileRenderChunkCacheConfig &config) const;
};

} // namespace iggy
