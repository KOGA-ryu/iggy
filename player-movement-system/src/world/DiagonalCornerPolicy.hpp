#pragma once

#include "world/Collision.hpp"
#include "world/Point.hpp"
#include "world/TileMap.hpp"

namespace dev {

class DiagonalCornerPolicy {
public:
	bool canStep(Point from, Point to, const TileMap &map, const Collision &collision) const;
};

} // namespace dev

