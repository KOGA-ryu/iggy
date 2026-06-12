#include "scene/level/LevelTileRenderChunkBuilder.hpp"

#include <algorithm>
#include <vector>

#include "scene/level/LevelTileDrawList.hpp"

namespace {

std::vector<iggy::TileCoord> TilesForChunk(iggy::TileCoord minTile, iggy::TileCoord maxTile)
{
	std::vector<iggy::TileCoord> tiles;
	const int width = (maxTile.x - minTile.x) + 1;
	const int height = (maxTile.y - minTile.y) + 1;
	tiles.reserve(static_cast<std::size_t>(width * height));

	for (int y = minTile.y; y <= maxTile.y; ++y) {
		for (int x = minTile.x; x <= maxTile.x; ++x)
			tiles.push_back({ x, y });
	}

	return tiles;
}

iggy::Aabb2 ChunkWorldBounds(iggy::TileCoord minTile, iggy::TileCoord maxTile)
{
	return {
		{ static_cast<float>(minTile.x), static_cast<float>(minTile.y) },
		{ static_cast<float>(maxTile.x + 1), static_cast<float>(maxTile.y + 1) },
	};
}

} // namespace

namespace iggy {

LevelTileRenderChunkBuildResult LevelTileRenderChunkBuilder::build(const LevelTileMap &map, LevelTileRenderChunkCoord coord, const LevelTileRenderChunkCacheConfig &config) const
{
	LevelTileRenderChunkBuildResult result;
	if (config.chunkWidth <= 0 || config.chunkHeight <= 0)
		return result;
	if (map.width <= 0 || map.height <= 0)
		return result;
	if (coord.x < 0 || coord.y < 0)
		return result;

	const TileCoord minTile { coord.x * config.chunkWidth, coord.y * config.chunkHeight };
	if (minTile.x >= map.width || minTile.y >= map.height)
		return result;

	const TileCoord maxTile {
		std::min(map.width - 1, minTile.x + config.chunkWidth - 1),
		std::min(map.height - 1, minTile.y + config.chunkHeight - 1),
	};
	const LevelTileDrawListResult drawList = LevelTileDrawList {}.build(map, TilesForChunk(minTile, maxTile));

	result.built = true;
	result.chunk = {
		coord,
		minTile,
		maxTile,
		ChunkWorldBounds(minTile, maxTile),
		LevelTileRenderCommands {}.build(drawList, config.tileCommands),
	};
	return result;
}

} // namespace iggy
