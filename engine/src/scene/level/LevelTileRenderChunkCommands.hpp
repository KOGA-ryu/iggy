#pragma once

#include <cstddef>
#include <vector>

#include "scene/level/LevelTileRenderChunkCache.hpp"
#include "scene/level/LevelTileRenderChunkVisibility.hpp"
#include "servers/render/RenderCommand2D.hpp"

namespace iggy {

struct LevelTileRenderChunkCommandResult {
	render::RenderCommandList2D commands;
	std::vector<std::size_t> usedChunkIndexes;
	std::vector<std::size_t> skippedChunkIndexes;
};

class LevelTileRenderChunkCommands {
public:
	[[nodiscard]] LevelTileRenderChunkCommandResult build(const LevelTileRenderChunkCache &cache, const std::vector<std::size_t> &chunkIndexes) const;
	[[nodiscard]] LevelTileRenderChunkCommandResult build(const LevelTileRenderChunkCache &cache, const LevelTileRenderChunkVisibilityResult &visibility) const;
};

} // namespace iggy
