#include "scene/level/LevelTileDrawList.hpp"

#include "scene/level/LevelGridQuery.hpp"

namespace iggy {

LevelTileDrawListResult LevelTileDrawList::build(const LevelTileMap &map, const std::vector<TileCoord> &visibleTiles) const
{
	LevelTileDrawListResult result;
	result.items.reserve(visibleTiles.size());

	for (TileCoord tile : visibleTiles) {
		if (!containsTile(map, tile))
			continue;

		const LevelTile *tileData = tileAt(map, tile);
		if (tileData == nullptr)
			continue;

		result.items.push_back({
			tile,
			tileBounds(tile),
			tileData->walkable,
			tileIndex(map, tile),
		});
	}

	return result;
}

} // namespace iggy
