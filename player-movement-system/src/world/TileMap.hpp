#pragma once

#include "world/Point.hpp"

namespace dev {

class TileMap {
public:
	bool isWalkable(Point tile) const;
	Point screenToTile(Point screenPosition) const;
};

} // namespace dev

