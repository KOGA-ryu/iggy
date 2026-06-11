#pragma once

#include "config/PathCostTuning.hpp"
#include "player/WalkPath.hpp"
#include "world/Collision.hpp"
#include "world/DiagonalCornerPolicy.hpp"
#include "world/Point.hpp"

namespace dev {

class TileMap;

class PathFinder {
public:
	explicit PathFinder(PathCostTuning tuning = {});

	WalkPath findPath(Point start, Point destination, const TileMap &map, const Collision &collision) const;

private:
	PathCostTuning tuning_;
	DiagonalCornerPolicy diagonalCornerPolicy_;
};

} // namespace dev
