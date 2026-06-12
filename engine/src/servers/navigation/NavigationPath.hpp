#pragma once

#include <vector>

#include "core/math/Vec2.hpp"
#include "scene/level/TileCoord.hpp"

namespace iggy::navigation {

enum class NavigationPathStatus {
	Found,
	NoPath,
	StartOutOfBounds,
	StartBlocked,
	DestinationRejected,
};

struct NavigationPath {
	NavigationPathStatus status = NavigationPathStatus::NoPath;
	std::vector<TileCoord> tiles;
	std::vector<Vec2> waypoints;

	[[nodiscard]] bool found() const;
};

} // namespace iggy::navigation
