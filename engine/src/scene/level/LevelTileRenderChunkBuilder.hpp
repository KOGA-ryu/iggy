#pragma once

#include "scene/level/LevelTileMap.hpp"
#include "scene/level/LevelTileRenderChunkCache.hpp"

namespace iggy {

struct LevelTileRenderChunkBuildResult {
	bool built = false;
	LevelTileRenderChunk chunk;
};

class LevelTileRenderChunkBuilder {
public:
	[[nodiscard]] LevelTileRenderChunkBuildResult build(const LevelTileMap &map, LevelTileRenderChunkCoord coord, const LevelTileRenderChunkCacheConfig &config) const;
};

} // namespace iggy
