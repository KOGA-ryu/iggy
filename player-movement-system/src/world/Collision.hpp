#pragma once

#include <vector>

#include "world/Point.hpp"

namespace dev {

class Collision {
public:
	void setBlocked(Point tile);
	bool blocksMovement(Point tile) const;

private:
	std::vector<Point> blockedTiles_;
};

} // namespace dev
