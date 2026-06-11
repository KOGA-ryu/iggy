#include "DiagonalCornerPolicy.hpp"

namespace dev {

namespace {

bool IsDiagonal(Point from, Point to)
{
	return from.x != to.x && from.y != to.y;
}

} // namespace

bool DiagonalCornerPolicy::canStep(Point from, Point to, const TileMap &map, const Collision &collision) const
{
	if (!IsDiagonal(from, to))
		return true;

	const Point sideA { from.x, to.y };
	const Point sideB { to.x, from.y };
	return (map.isWalkable(sideA) && !collision.blocksMovement(sideA))
	    || (map.isWalkable(sideB) && !collision.blocksMovement(sideB));
}

} // namespace dev

