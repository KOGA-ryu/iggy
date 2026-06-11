#include "Collision.hpp"

namespace dev {

bool Collision::blocksMovement(Point tile) const
{
	return tile.x < 0 || tile.y < 0;
}

} // namespace dev

