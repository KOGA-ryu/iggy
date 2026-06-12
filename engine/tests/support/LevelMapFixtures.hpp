#pragma once

#include <string_view>
#include <vector>

#include "scene/level/LevelTileMap.hpp"
#include "scene/level/TileCoord.hpp"

namespace iggy::test {

inline LevelTileMap MapFromRows(std::vector<std::string_view> rows)
{
	LevelTileMap map;
	map.height = static_cast<int>(rows.size());
	map.width = rows.empty() ? 0 : static_cast<int>(rows.front().size());
	for (std::string_view row : rows) {
		for (char cell : row)
			map.tiles.push_back({ cell != '#' });
	}
	return map;
}

inline bool SameTile(TileCoord actual, int x, int y)
{
	return actual.x == x && actual.y == y;
}

} // namespace iggy::test
