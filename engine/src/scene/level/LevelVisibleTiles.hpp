#pragma once

#include <vector>

#include "core/math/Aabb2.hpp"
#include "scene/level/LevelTileMap.hpp"
#include "scene/level/TileCoord.hpp"

namespace iggy {

struct LevelVisibleTilesResult {
	bool hasTiles = false;
	TileCoord minTile;
	TileCoord maxTile;
	std::vector<TileCoord> tiles;
};

class LevelVisibleTiles {
public:
	[[nodiscard]] LevelVisibleTilesResult query(const LevelTileMap &map, Aabb2 worldBounds) const;
};

} // namespace iggy
