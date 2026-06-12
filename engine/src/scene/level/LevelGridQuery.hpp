#pragma once

#include <cstddef>

#include "core/math/Aabb2.hpp"
#include "scene/level/LevelTileMap.hpp"
#include "scene/level/TileCoord.hpp"

namespace iggy {

[[nodiscard]] inline Aabb2 tileBounds(TileCoord tile)
{
	return { { static_cast<float>(tile.x), static_cast<float>(tile.y) }, { static_cast<float>(tile.x + 1), static_cast<float>(tile.y + 1) } };
}

[[nodiscard]] inline bool containsTile(const LevelTileMap &map, TileCoord tile)
{
	return map.contains(tile.x, tile.y);
}

[[nodiscard]] inline const LevelTile *tileAt(const LevelTileMap &map, TileCoord tile)
{
	return map.tileAt(tile.x, tile.y);
}

[[nodiscard]] inline bool isWalkable(const LevelTileMap &map, TileCoord tile)
{
	const LevelTile *tileData = tileAt(map, tile);
	return tileData != nullptr && tileData->walkable;
}

[[nodiscard]] inline std::size_t tileIndex(const LevelTileMap &map, TileCoord tile)
{
	return static_cast<std::size_t>(tile.y) * static_cast<std::size_t>(map.width) + static_cast<std::size_t>(tile.x);
}

} // namespace iggy
