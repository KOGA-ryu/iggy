#include "scene/level/LevelTileRenderChunkCommands.hpp"

#include "servers/render/RenderCommandList2DComposer.hpp"

namespace iggy {

LevelTileRenderChunkCommandResult LevelTileRenderChunkCommands::build(const LevelTileRenderChunkCache &cache, const std::vector<std::size_t> &chunkIndexes) const
{
	LevelTileRenderChunkCommandResult result;
	render::RenderCommandList2DComposer composer;

	for (std::size_t index : chunkIndexes) {
		if (index >= cache.chunks.size()) {
			result.skippedChunkIndexes.push_back(index);
			continue;
		}

		composer.append(result.commands, cache.chunks[index].commands);
		result.usedChunkIndexes.push_back(index);
	}

	return result;
}

LevelTileRenderChunkCommandResult LevelTileRenderChunkCommands::build(const LevelTileRenderChunkCache &cache, const LevelTileRenderChunkVisibilityResult &visibility) const
{
	return build(cache, visibility.chunkIndexes);
}

} // namespace iggy
