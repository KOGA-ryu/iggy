#include "scene/level/LevelTileRenderChunkCache.hpp"

#include "scene/level/LevelTileRenderChunkBuilder.hpp"

namespace iggy {

LevelTileRenderChunkCacheBuildResult LevelTileRenderChunkCacheBuilder::build(const LevelTileMap &map, const LevelTileRenderChunkCacheConfig &config) const
{
	LevelTileRenderChunkCacheBuildResult result;
	if (config.chunkWidth <= 0 || config.chunkHeight <= 0) {
		result.issues.push_back({ LevelTileRenderChunkCacheIssueCode::InvalidChunkSize, config.chunkWidth, config.chunkHeight });
		return result;
	}

	result.built = true;
	if (map.width <= 0 || map.height <= 0)
		return result;

	const int chunkCountX = (map.width + config.chunkWidth - 1) / config.chunkWidth;
	const int chunkCountY = (map.height + config.chunkHeight - 1) / config.chunkHeight;
	result.cache.chunks.reserve(static_cast<std::size_t>(chunkCountX * chunkCountY));

	for (int chunkY = 0; chunkY < chunkCountY; ++chunkY) {
		for (int chunkX = 0; chunkX < chunkCountX; ++chunkX) {
			const LevelTileRenderChunkBuildResult chunk = LevelTileRenderChunkBuilder {}.build(map, { chunkX, chunkY }, config);
			if (chunk.built)
				result.cache.chunks.push_back(chunk.chunk);
		}
	}

	return result;
}

} // namespace iggy
