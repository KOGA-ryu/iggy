#pragma once

#include <cstddef>
#include <vector>

#include "core/math/Aabb2.hpp"
#include "scene/level/LevelTileRenderChunkCache.hpp"

namespace iggy {

struct LevelTileRenderChunkVisibilityResult {
	bool hasChunks = false;
	std::vector<std::size_t> chunkIndexes;
};

class LevelTileRenderChunkVisibility {
public:
	[[nodiscard]] LevelTileRenderChunkVisibilityResult query(const LevelTileRenderChunkCache &cache, Aabb2 worldBounds) const;
};

} // namespace iggy
