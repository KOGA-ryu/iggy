#pragma once

#include <cstddef>

#include "core/math/Vec2.hpp"
#include "servers/navigation/NavigationPath.hpp"

namespace iggy::navigation {

struct NavigationPathFollowState {
	std::size_t waypointIndex = 0;
	bool completed = false;
};

struct NavigationPathFollowResult {
	Vec2 position;
	std::size_t waypointIndex = 0;
	bool moved = false;
	bool completed = false;
};

class NavigationPathFollower {
public:
	[[nodiscard]] NavigationPathFollowResult step(const NavigationPath &path, NavigationPathFollowState state, Vec2 currentPosition, float maxDistance) const;
};

} // namespace iggy::navigation
