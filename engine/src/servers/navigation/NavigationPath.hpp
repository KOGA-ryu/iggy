#pragma once

#include <vector>

#include "core/math/Vec2.hpp"

namespace iggy::navigation {

enum class NavigationPathStatus {
	Found,
	NoPath,
	StartOutOfBounds,
	StartBlocked,
	DestinationRejected,
};

struct NavigationPathTile {
	int x = -1;
	int y = -1;
};

struct NavigationPath {
	NavigationPathStatus status = NavigationPathStatus::NoPath;
	std::vector<NavigationPathTile> tiles;
	std::vector<Vec2> waypoints;

	[[nodiscard]] bool found() const;
};

} // namespace iggy::navigation
