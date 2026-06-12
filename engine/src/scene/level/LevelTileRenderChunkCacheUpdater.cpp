#include "scene/level/LevelTileRenderChunkCacheUpdater.hpp"

#include <cstddef>

#include "scene/level/LevelTileRenderChunkBuilder.hpp"

namespace {

bool SameChunk(iggy::LevelTileRenderChunkCoord left, iggy::LevelTileRenderChunkCoord right)
{
	return left.x == right.x && left.y == right.y;
}

bool ContainsChunk(const std::vector<iggy::LevelTileRenderChunkCoord> &chunks, iggy::LevelTileRenderChunkCoord chunk)
{
	for (iggy::LevelTileRenderChunkCoord existing : chunks) {
		if (SameChunk(existing, chunk))
			return true;
	}
	return false;
}

std::vector<iggy::LevelTileRenderChunkCoord> UniqueChunks(const std::vector<iggy::LevelTileRenderChunkCoord> &chunks)
{
	std::vector<iggy::LevelTileRenderChunkCoord> unique;
	unique.reserve(chunks.size());
	for (iggy::LevelTileRenderChunkCoord chunk : chunks) {
		if (!ContainsChunk(unique, chunk))
			unique.push_back(chunk);
	}
	return unique;
}

iggy::LevelTileRenderChunk *FindChunk(iggy::LevelTileRenderChunkCache &cache, iggy::LevelTileRenderChunkCoord coord)
{
	for (iggy::LevelTileRenderChunk &chunk : cache.chunks) {
		if (SameChunk(chunk.coord, coord))
			return &chunk;
	}
	return nullptr;
}

} // namespace

namespace iggy {

LevelTileRenderChunkCacheUpdateResult LevelTileRenderChunkCacheUpdater::update(
	const LevelTileRenderChunkCache &current,
	const LevelTileMap &map,
	const LevelTileRenderChunkCacheConfig &config,
	const std::vector<LevelTileRenderChunkCoord> &dirtyChunks) const
{
	LevelTileRenderChunkCacheUpdateResult result;
	if (config.chunkWidth <= 0 || config.chunkHeight <= 0) {
		result.issues.push_back({ LevelTileRenderChunkCacheUpdateIssueCode::InvalidChunkSize, config.chunkWidth, config.chunkHeight });
		return result;
	}

	result.updated = true;
	result.cache = current;
	for (LevelTileRenderChunkCoord dirty : UniqueChunks(dirtyChunks)) {
		LevelTileRenderChunk *existing = FindChunk(result.cache, dirty);
		if (existing == nullptr) {
			result.skippedChunks.push_back(dirty);
			continue;
		}

		const LevelTileRenderChunkBuildResult rebuilt = LevelTileRenderChunkBuilder {}.build(map, dirty, config);
		if (!rebuilt.built) {
			result.skippedChunks.push_back(dirty);
			continue;
		}

		*existing = rebuilt.chunk;
		result.rebuiltChunks.push_back(dirty);
	}

	return result;
}

} // namespace iggy
