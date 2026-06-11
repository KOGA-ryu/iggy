#pragma once

#include <vector>

#include "world/Point.hpp"

namespace dev {

class TileMap {
public:
	void setBlocked(Point tile);
	bool isWalkable(Point tile) const;
	Point screenToTile(Point screenPosition) const;

private:
	std::vector<Point> blockedTiles_;
};

} // namespace dev
