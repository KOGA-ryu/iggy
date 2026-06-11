#include "TileMap.hpp"

namespace dev {

void TileMap::setBlocked(Point tile)
{
	blockedTiles_.push_back(tile);
}

bool TileMap::isWalkable(Point tile) const
{
	if (tile.x < 0 || tile.y < 0)
		return false;
	for (Point blocked : blockedTiles_) {
		if (blocked == tile)
			return false;
	}
	return true;
}

Point TileMap::screenToTile(Point screenPosition) const
{
	return { screenPosition.x / 32, screenPosition.y / 32 };
}

} // namespace dev
