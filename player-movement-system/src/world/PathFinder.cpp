#include "PathFinder.hpp"

#include <cstdlib>

#include "world/TileMap.hpp"

namespace dev {

PathFinder::PathFinder(PathCostTuning tuning)
    : tuning_(tuning)
{
}

WalkPath PathFinder::findPath(Point start, Point destination, const TileMap &map, const Collision &collision) const
{
	WalkPath path;
	if (start == destination || !map.isWalkable(destination))
		return path;

	Point cursor = start;
	while (!(cursor == destination) && path.size() < MaxWalkPathLength) {
		const int dx = destination.x - cursor.x;
		const int dy = destination.y - cursor.y;
		const bool preferAxis = tuning_.diagonalStepCost > tuning_.axisAlignedStepCost
		    && std::abs(dx) == std::abs(dy);

		Point next = cursor;
		if (!preferAxis && dx != 0)
			next.x += dx > 0 ? 1 : -1;
		if (dy != 0)
			next.y += dy > 0 ? 1 : -1;
		if (preferAxis && dx != 0)
			next.x += dx > 0 ? 1 : -1;

		if (!map.isWalkable(next) || collision.blocksMovement(next))
			break;
		if (!diagonalCornerPolicy_.canStep(cursor, next, map, collision))
			break;
		if (!path.pushStep(next))
			break;
		cursor = next;
	}

	return path;
}

} // namespace dev
