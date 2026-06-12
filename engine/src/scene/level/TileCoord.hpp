#pragma once

#include <cmath>

#include "core/math/Vec2.hpp"

namespace iggy {

struct TileCoord {
	int x = -1;
	int y = -1;
};

inline bool operator==(TileCoord left, TileCoord right)
{
	return left.x == right.x && left.y == right.y;
}

inline bool operator!=(TileCoord left, TileCoord right)
{
	return !(left == right);
}

inline bool sameTile(TileCoord left, TileCoord right)
{
	return left == right;
}

inline TileCoord tileForPoint(Vec2 point)
{
	return { static_cast<int>(std::floor(point.x)), static_cast<int>(std::floor(point.y)) };
}

inline Vec2 tileCenter(TileCoord tile)
{
	return { static_cast<float>(tile.x) + 0.5F, static_cast<float>(tile.y) + 0.5F };
}

} // namespace iggy
