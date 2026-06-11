#include "Collision.hpp"

namespace dev {

void Collision::setBlocked(Point tile)
{
	blockedTiles_.push_back(tile);
}

bool Collision::blocksMovement(Point tile) const
{
	if (tile.x < 0 || tile.y < 0)
		return true;
	for (Point blocked : blockedTiles_) {
		if (blocked == tile)
			return true;
	}
	return false;
}

} // namespace dev
