#pragma once

#include <cstddef>
#include <vector>

#include "core/math/Aabb2.hpp"
#include "scene/level/LevelTileMap.hpp"
#include "scene/level/TileCoord.hpp"

namespace iggy {

struct LevelTileDrawItem {
	TileCoord tile;
	Aabb2 worldBounds;
	bool walkable = false;
	std::size_t tileIndex = 0;
};

struct LevelTileDrawListResult {
	std::vector<LevelTileDrawItem> items;
};

class LevelTileDrawList {
public:
	[[nodiscard]] LevelTileDrawListResult build(const LevelTileMap &map, const std::vector<TileCoord> &visibleTiles) const;
};

} // namespace iggy
