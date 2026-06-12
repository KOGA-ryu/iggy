#include "scene/level/LevelVisibleTiles.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace iggy {

namespace {

struct AxisRange {
	int min = 0;
	int max = -1;
};

AxisRange TileRangeForAxis(float rawMin, float rawMax, int tileCount)
{
	const float normalizedMin = std::min(rawMin, rawMax);
	const float normalizedMax = std::max(rawMin, rawMax);
	const int minTile = static_cast<int>(std::floor(normalizedMin));
	const int maxTile = normalizedMin == normalizedMax
	    ? minTile
	    : static_cast<int>(std::floor(std::nextafter(normalizedMax, -std::numeric_limits<float>::infinity())));

	if (maxTile < 0 || minTile >= tileCount)
		return {};

	return {
		std::max(0, minTile),
		std::min(tileCount - 1, maxTile),
	};
}

} // namespace

LevelVisibleTilesResult LevelVisibleTiles::query(const LevelTileMap &map, Aabb2 worldBounds) const
{
	LevelVisibleTilesResult result;
	if (map.width <= 0 || map.height <= 0 || map.tiles.empty())
		return result;

	const AxisRange xRange = TileRangeForAxis(worldBounds.min.x, worldBounds.max.x, map.width);
	const AxisRange yRange = TileRangeForAxis(worldBounds.min.y, worldBounds.max.y, map.height);
	if (xRange.min > xRange.max || yRange.min > yRange.max)
		return result;

	result.hasTiles = true;
	result.minTile = { xRange.min, yRange.min };
	result.maxTile = { xRange.max, yRange.max };
	result.tiles.reserve(static_cast<std::size_t>(xRange.max - xRange.min + 1) * static_cast<std::size_t>(yRange.max - yRange.min + 1));
	for (int y = yRange.min; y <= yRange.max; ++y) {
		for (int x = xRange.min; x <= xRange.max; ++x)
			result.tiles.push_back({ x, y });
	}

	return result;
}

} // namespace iggy
