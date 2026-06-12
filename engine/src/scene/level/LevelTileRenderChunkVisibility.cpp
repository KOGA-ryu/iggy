#include "scene/level/LevelTileRenderChunkVisibility.hpp"

#include <algorithm>

namespace {

iggy::Aabb2 Normalize(iggy::Aabb2 bounds)
{
	return {
		{ std::min(bounds.min.x, bounds.max.x), std::min(bounds.min.y, bounds.max.y) },
		{ std::max(bounds.min.x, bounds.max.x), std::max(bounds.min.y, bounds.max.y) },
	};
}

bool IsPointBounds(iggy::Aabb2 bounds)
{
	return bounds.min.x == bounds.max.x && bounds.min.y == bounds.max.y;
}

bool HalfOpenOverlap(iggy::Aabb2 query, iggy::Aabb2 chunk)
{
	return query.min.x < chunk.max.x && query.max.x > chunk.min.x && query.min.y < chunk.max.y && query.max.y > chunk.min.y;
}

} // namespace

namespace iggy {

LevelTileRenderChunkVisibilityResult LevelTileRenderChunkVisibility::query(const LevelTileRenderChunkCache &cache, Aabb2 worldBounds) const
{
	LevelTileRenderChunkVisibilityResult result;
	const Aabb2 queryBounds = Normalize(worldBounds);
	result.chunkIndexes.reserve(cache.chunks.size());

	for (std::size_t index = 0; index < cache.chunks.size(); ++index) {
		const LevelTileRenderChunk &chunk = cache.chunks[index];
		if (IsPointBounds(queryBounds) ? chunk.worldBounds.contains(queryBounds.min) : HalfOpenOverlap(queryBounds, chunk.worldBounds))
			result.chunkIndexes.push_back(index);
	}

	result.hasChunks = !result.chunkIndexes.empty();
	return result;
}

} // namespace iggy
