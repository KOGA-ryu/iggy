#include "TileMap.hpp"

namespace dev {

bool TileMap::isWalkable(Point tile) const
{
	return tile.x >= 0 && tile.y >= 0;
}

Point TileMap::screenToTile(Point screenPosition) const
{
	return { screenPosition.x / 32, screenPosition.y / 32 };
}

} // namespace dev

