#include "scene/level/LevelTileRenderDirtyChunks.hpp"

namespace {

int FloorDiv(int value, int divisor)
{
	int quotient = value / divisor;
	const int remainder = value % divisor;
	if (remainder != 0 && ((remainder < 0) != (divisor < 0)))
		--quotient;
	return quotient;
}

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

} // namespace

namespace iggy {

LevelTileRenderDirtyChunksResult LevelTileRenderDirtyChunks::query(const std::vector<TileCoord> &changedTiles, const LevelTileRenderDirtyChunksConfig &config) const
{
	LevelTileRenderDirtyChunksResult result;
	if (config.chunkWidth <= 0 || config.chunkHeight <= 0) {
		result.issues.push_back({ LevelTileRenderDirtyChunksIssueCode::InvalidChunkSize, config.chunkWidth, config.chunkHeight });
		return result;
	}

	result.queried = true;
	for (TileCoord tile : changedTiles) {
		const LevelTileRenderChunkCoord chunk {
			FloorDiv(tile.x, config.chunkWidth),
			FloorDiv(tile.y, config.chunkHeight),
		};
		if (!ContainsChunk(result.chunks, chunk))
			result.chunks.push_back(chunk);
	}

	return result;
}

LevelTileRenderDirtyChunksResult LevelTileRenderDirtyChunks::query(const std::vector<TileCoord> &changedTiles, const LevelTileRenderChunkCacheConfig &config) const
{
	const LevelTileRenderDirtyChunksConfig dirtyConfig { config.chunkWidth, config.chunkHeight };
	return query(changedTiles, dirtyConfig);
}

} // namespace iggy
